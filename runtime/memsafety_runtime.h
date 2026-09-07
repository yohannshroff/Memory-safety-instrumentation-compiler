/*
 * memsafety_runtime.h - runtime support library for the memsafety instrumentation
 * pass.
 *
 * The instrumentation pass (src/) inserts calls to the functions declared here
 * around array accesses, pointer dereferences and heap operations. This library
 * is plain C with no LLVM dependency and is compiled to a static archive
 * (libmemsafety_runtime.a) that instrumented programs link against.
 *
 * Violation policy: the first detected violation is printed to stderr by
 * report_violation() and the process is then terminated with abort(), so an
 * instrumented program that hits a violation exits with a non-zero status
 * (typically 134 = 128 + SIGABRT). This is a deliberate choice - it makes the
 * pass/fail decision in the benchmark harness unambiguous and stops the program
 * before the unsafe access actually executes.
 */
#ifndef MEMSAFETY_RUNTIME_H
#define MEMSAFETY_RUNTIME_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Heap bookkeeping. heap_register records an allocation returned by
 * malloc/calloc; heap_release is called just before free(). A release of a
 * pointer that was never registered is an "invalid free"; a release of a
 * pointer already marked freed is a "double free". Freed entries are retained
 * (not erased) so that ptrcheck() can still recognise use-after-free.
 */
void heap_register(void *ptr, size_t size, const char *loc);
void heap_release(void *ptr, const char *loc);

/*
 * boundscheck validates that `index` lies in [0, len) for an array whose base
 * is `base` and whose element size is `elem_size` bytes. Out-of-range indices
 * (including negative ones) are reported as an out-of-bounds access.
 */
void boundscheck(void *base, size_t elem_size, size_t len, long index,
                 const char *loc);

/*
 * ptrcheck validates a pointer that is about to be dereferenced. A NULL pointer
 * is reported as a null dereference; a pointer that matches a heap entry marked
 * freed is reported as a use-after-free.
 */
void ptrcheck(void *ptr, const char *loc);

/*
 * report_violation prints a diagnostic of the form
 *   [VIOLATION] <type> at <loc>, addr=<addr>
 * to stderr and terminates the process with abort(). `loc` may be NULL or
 * "<unknown>" when the pass could not recover a source location.
 */
void report_violation(const char *type, const char *loc, const void *addr);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* MEMSAFETY_RUNTIME_H */
