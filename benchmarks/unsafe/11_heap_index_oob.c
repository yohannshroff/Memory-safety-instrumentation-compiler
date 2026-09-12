#include <stdlib.h>
int main(void) {
    int *p = malloc(4 * sizeof(int));
    if (!p) return 1;
    p[4] = 1;               // heap OOB via subscript (not pointer arithmetic)
    free(p);
    return 0;
}
