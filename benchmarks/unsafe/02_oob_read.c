#include <stdio.h>
int main(void) {
    int a[4] = {10, 20, 30, 40};
    printf("%d\n", a[4]);   // OOB read
    return 0;
}
