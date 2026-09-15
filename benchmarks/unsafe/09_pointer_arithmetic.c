#include <stdlib.h>
int main(void) {
    int *p = malloc(4 * sizeof(int));
    if (!p) return 1;
    int *q = p + 2;
    *q = 7;                 // Valid
    *(p + 4) = 9;           // out Of Bounds
    free(p);
    return 0;
}
