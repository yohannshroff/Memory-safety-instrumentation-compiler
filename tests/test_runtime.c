/*
 * test_runtime.c - direct unit tests for runtime/memsafety_runtime.c.
 *
 * No LLVM / no instrumentation pass is involved here: the runtime API is
 * exercised the same way the pass-inserted calls would exercise it.
 *
 * "Happy path" calls are made inline and simply must not abort. Each violation
 * path aborts the process (report_violation calls abort()), so those are run in
 * a forked child whose stderr is captured; the parent asserts the child died
 * from SIGABRT and printed the expected "[VIOLATION] <type>" line.
 */
#include "memsafety_runtime.h"

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static int g_tests = 0;
static int g_failures = 0;

#define CHECK(cond, name)                                                       \
  do {                                                                          \
    ++g_tests;                                                                  \
    if (cond) {                                                                 \
      printf("  ok   - %s\n", (name));                                          \
    } else {                                                                    \
      printf("  FAIL - %s  (%s:%d)\n", (name), __FILE__, __LINE__);             \
      ++g_failures;                                                             \
    }                                                                          \
  } while (0)

/*
 * Run `fn` in a child process, capture its stderr, and return 1 iff the child
 * terminated via SIGABRT and its stderr contained both "[VIOLATION]" and the
 * substring `want_type`.
 */
static int aborts_with_violation(void (*fn)(void), const char *want_type) {
  fflush(stdout); /* don't let the child inherit and re-flush our buffer */

  int fds[2];
  if (pipe(fds) != 0) {
    perror("pipe");
    return 0;
  }

  pid_t pid = fork();
  if (pid < 0) {
    perror("fork");
    return 0;
  }

  if (pid == 0) {
    /* child: redirect stderr into the pipe, run the offending call */
    close(fds[0]);
    dup2(fds[1], STDERR_FILENO);
    close(fds[1]);
    fn();
    _exit(0); /* fn did not abort - the parent will flag this */
  }

  /* parent: drain the pipe, then reap */
  close(fds[1]);
  char buf[1024];
  size_t used = 0;
  ssize_t n;
  while (used + 1 < sizeof(buf) &&
         (n = read(fds[0], buf + used, sizeof(buf) - 1 - used)) > 0) {
    used += (size_t)n;
  }
  buf[used] = '\0';
  close(fds[0]);

  int status = 0;
  waitpid(pid, &status, 0);

  int signalled_abort = WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT;
  int has_marker = strstr(buf, "[VIOLATION]") != NULL;
  int has_type = strstr(buf, want_type) != NULL;

  if (!signalled_abort || !has_marker || !has_type) {
    printf("    (child status=0x%x, stderr=%.*s)\n", status, (int)used, buf);
  }
  return signalled_abort && has_marker && has_type;
}

/* ---- violation-path thunks (each is run inside a forked child) ---------- */

static void v_invalid_free(void) {
  int local = 0;
  heap_release(&local, "test:invalid_free");
}

static void v_double_free(void) {
  void *p = malloc(16);
  heap_register(p, 16, "test:double_free");
  heap_release(p, "test:double_free");
  heap_release(p, "test:double_free"); /* second release -> double free */
}

static void v_oob_high(void) {
  int a[4];
  boundscheck(a, sizeof(int), 4, 4, "test:oob_high");
}

static void v_oob_negative(void) {
  int a[4];
  boundscheck(a, sizeof(int), 4, -1, "test:oob_negative");
}

static void v_null_deref(void) { ptrcheck(NULL, "test:null_deref"); }

static void v_use_after_free(void) {
  void *p = malloc(sizeof(int));
  heap_register(p, sizeof(int), "test:uaf");
  heap_release(p, "test:uaf");
  ptrcheck(p, "test:uaf"); /* dereference-after-free */
}

/* ----------------------------------------------------------------------- */

int main(void) {
  printf("test_runtime\n");

  /* --- happy paths: must NOT abort --- */
  {
    int a[8];
    boundscheck(a, sizeof(int), 8, 0, "test:in_bounds_low");
    boundscheck(a, sizeof(int), 8, 7, "test:in_bounds_high");
    CHECK(1, "boundscheck accepts in-range indices");
  }

  {
    int stack_obj = 42;
    ptrcheck(&stack_obj, "test:live_ptr");
    CHECK(1, "ptrcheck accepts a non-null, non-freed pointer");
  }

  {
    void *p = malloc(32);
    heap_register(p, 32, "test:valid_cycle");
    ptrcheck(p, "test:valid_cycle");
    heap_release(p, "test:valid_cycle");
    free(p);
    CHECK(1, "register / use / release cycle is accepted");
  }

  {
    /* force at least one table growth: default capacity is 64 */
    void *keep[128];
    for (int i = 0; i < 128; ++i) {
      keep[i] = malloc(8);
      heap_register(keep[i], 8, "test:grow");
    }
    int all_found = 1;
    for (int i = 0; i < 128; ++i) {
      /* a live registered pointer must not trip ptrcheck */
      ptrcheck(keep[i], "test:grow");
    }
    for (int i = 0; i < 128; ++i) {
      heap_release(keep[i], "test:grow");
      free(keep[i]);
    }
    CHECK(all_found, "metadata table survives growth past initial capacity");
  }

  heap_register(NULL, 0, "test:null_register");
  heap_release(NULL, "test:null_release");
  CHECK(1, "NULL register / release are no-ops");

  /* --- violation paths: must abort with the right diagnostic --- */
  CHECK(aborts_with_violation(v_invalid_free, "invalid free"),
        "heap_release on unregistered pointer -> invalid free");
  CHECK(aborts_with_violation(v_double_free, "double free"),
        "second heap_release -> double free");
  CHECK(aborts_with_violation(v_oob_high, "out-of-bounds access"),
        "boundscheck index == len -> out-of-bounds access");
  CHECK(aborts_with_violation(v_oob_negative, "out-of-bounds access"),
        "boundscheck negative index -> out-of-bounds access");
  CHECK(aborts_with_violation(v_null_deref, "null dereference"),
        "ptrcheck(NULL) -> null dereference");
  CHECK(aborts_with_violation(v_use_after_free, "use after free"),
        "ptrcheck on freed pointer -> use after free");

  printf("\n%d tests, %d failures\n", g_tests, g_failures);
  return g_failures == 0 ? 0 : 1;
}
