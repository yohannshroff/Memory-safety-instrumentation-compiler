#include <stdlib.h>
int main(void) {
    int x = 10;
    free(&x);               // invalid free
    return 0;
}
