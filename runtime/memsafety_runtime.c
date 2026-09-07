/*
 * memsafety_runtime.c - implementation of the memsafety runtime support library.
 *
 * Data structure: a single open-addressing hash table (linear probing) mapping a
 * heap base pointer to { size, freed flag, allocation site }. The table grows
 * and rehashes when it passes a 70% load factor. At benchmark scale (tens of
 * live allocations) this is comfortably fast; a plain linear array would also
 * do, but the hash table keeps lookups O(1) without extra code once written.
 *
 * The runtime is single-threaded by design (see README "Known Limitations").
 */
#include "memsafety_runtime.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* ------------------------------------------------------------------------- */
/* Allocation metadata table                                                 */
/* ------------------------------------------------------------------------- */

typedef struct {
  void *base;             /* allocation base pointer; NULL => empty slot     */
  size_t size;            /* usable size in bytes                            */
  int freed;              /* 1 once heap_release has seen this pointer       */
  const char *alloc_site; /* source location string from the pass, or NULL  */
} entry_t;

static entry_t *g_table = NULL;
static size_t g_capacity = 0; /* always a power of two, or 0 before init     */
static size_t g_count = 0;    /* number of occupied slots (freed or not)     */

#define MS_INITIAL_CAPACITY 64u

static size_t ptr_hash(const void *p) {
  /* splitmix64 finaliser on the pointer bits - good spread for aligned ptrs */
  uint64_t x = (uint64_t)(uintptr_t)p;
  x += 0x9E3779B97F4A7C15ull;
  x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
  x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
  x = x ^ (x >> 31);
  return (size_t)x;
}

static void table_init(void) {
  g_capacity = MS_INITIAL_CAPACITY;
  g_table = (entry_t *)calloc(g_capacity, sizeof(entry_t));
  if (!g_table) {
    fprintf(stderr, "[memsafety] fatal: out of memory allocating metadata "
                    "table\n");
    abort();
  }
  g_count = 0;
}

/* Find the slot for `base`: either the slot already holding it, or the first
 * empty slot on its probe sequence (for insertion). Returns an index; the
 * caller checks whether g_table[idx].base matches. */
static size_t table_slot(void *base) {
  size_t mask = g_capacity - 1;
  size_t idx = ptr_hash(base) & mask;
  while (g_table[idx].base != NULL && g_table[idx].base != base) {
    idx = (idx + 1) & mask;
  }
  return idx;
}

static void table_grow(void) {
  entry_t *old = g_table;
  size_t old_cap = g_capacity;

  g_capacity *= 2;
  g_table = (entry_t *)calloc(g_capacity, sizeof(entry_t));
  if (!g_table) {
    fprintf(stderr, "[memsafety] fatal: out of memory growing metadata "
                    "table\n");
    abort();
  }
  g_count = 0;

  for (size_t i = 0; i < old_cap; ++i) {
    if (old[i].base != NULL) {
      size_t idx = table_slot(old[i].base);
      g_table[idx] = old[i];
      ++g_count;
    }
  }
  free(old);
}

static entry_t *table_lookup(void *base) {
  if (g_capacity == 0 || base == NULL)
    return NULL;
  size_t idx = table_slot(base);
  return g_table[idx].base == base ? &g_table[idx] : NULL;
}

/* ------------------------------------------------------------------------- */
/* Public API                                                                */
/* ------------------------------------------------------------------------- */

void report_violation(const char *type, const char *loc, const void *addr) {
  fflush(stdout);
  fprintf(stderr, "[VIOLATION] %s at %s, addr=%p\n", type ? type : "unknown",
          (loc && *loc) ? loc : "<unknown>", addr);
  fflush(stderr);
  abort();
}

void heap_register(void *ptr, size_t size, const char *loc) {
  if (ptr == NULL)
    return; /* allocation failed; nothing to track */

  if (g_capacity == 0)
    table_init();
  if ((g_count + 1) * 10 >= g_capacity * 7)
    table_grow();

  size_t idx = table_slot(ptr);
  if (g_table[idx].base == NULL)
    ++g_count;

  g_table[idx].base = ptr;
  g_table[idx].size = size;
  g_table[idx].freed = 0;
  g_table[idx].alloc_site = loc;
}

void heap_release(void *ptr, const char *loc) {
  if (ptr == NULL)
    return; /* free(NULL) is well defined and harmless */

  entry_t *e = table_lookup(ptr);
  if (e == NULL) {
    report_violation("invalid free", loc, ptr);
    return; /* not reached: report_violation aborts */
  }
  if (e->freed) {
    report_violation("double free", loc, ptr);
    return;
  }
  e->freed = 1;
}

void boundscheck(void *base, size_t elem_size, size_t len, long index,
                 const char *loc) {
  (void)elem_size; /* reserved for byte-accurate diagnostics */
  if (index < 0 || (size_t)index >= len) {
    report_violation("out-of-bounds access", loc, base);
  }
}

void ptrcheck(void *ptr, const char *loc) {
  if (ptr == NULL) {
    report_violation("null dereference", loc, ptr);
    return;
  }
  entry_t *e = table_lookup(ptr);
  if (e != NULL && e->freed) {
    report_violation("use after free", loc, ptr);
  }
}
