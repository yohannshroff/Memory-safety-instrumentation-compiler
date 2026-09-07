#include <stdlib.h>
int main(void) {
    int *p = malloc(sizeof(int));
    if (!p) return 1;
    *p = 10;
    free(p);
    *p = 20;                // use after free
    return 0;
}
