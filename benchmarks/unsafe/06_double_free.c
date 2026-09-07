#include <stdlib.h>
int main(void) {
    int *p = malloc(16);
    if (!p) return 1;
    free(p);
    free(p);                // double free
    return 0;
}
