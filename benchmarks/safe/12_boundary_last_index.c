#include <stdlib.h>
int main(void) {
    int a[4] = {1, 2, 3, 4};
    int last_stack = a[3];         // exact last valid stack index

    int *p = malloc(4 * sizeof(int));
    if (!p) return 1;
    for (int i = 0; i < 4; ++i) p[i] = i;
    int last_heap = p[3];          // exact last valid heap index
    free(p);

    return (last_stack + last_heap) == (4 + 3) ? 0 : 1;
}
