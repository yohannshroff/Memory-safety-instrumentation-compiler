#include <stdio.h>
#include <stdlib.h>
int main(void) {
    int a[2] = {1, 2};
    int *p = malloc(2 * sizeof(int));
    if (!p) return 1;
    p[0] = 5;               // valid
    printf("%d\n", a[2]);   // OOB read
    free(p);
    return 0;
}
