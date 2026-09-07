int main(void) {
    int a[4] = {0};
    a[4] = 42;              // OOB write
    return 0;
}
