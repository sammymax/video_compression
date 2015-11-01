#include <stdio.h>
int dosfaa(int *x) {
    return __sync_fetch_and_add(x, 1);
}
int main(int argc __attribute__((unused)),
         char *argv[]  __attribute__((unused))) {
    int x = 0;
    int y = dosfaa(&x);
    printf("x=%d y=%d\n", x, y);
    return 0;
}
