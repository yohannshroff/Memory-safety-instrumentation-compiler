#include <stdlib.h>
int main(void) {
    int *p = malloc(4 * sizeof(int));
    if (!p) return 1;
    for (int i = 0; i < 4; ++i) p[i] = i;
    free(p);
    return 0;
}
