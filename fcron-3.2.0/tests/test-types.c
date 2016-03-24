#include <stdio.h>
#include <sys/types.h>
#include <ctype.h>
#include <limits.h>


int main(int argc, char **argv) {

    long a = LONG_MAX-10;
    unsigned long b = a;
    long c = b;

    printf("a=%ld %lu %lx b=%ld %lu %lx c=%ld %lu %lx LONG_MAX=%ld %lu %lx ULONG_MAX=%ld %lu %lx\n", a,a,a,b,b,b,c,c,c,LONG_MAX,LONG_MAX,LONG_MAX,ULONG_MAX,ULONG_MAX,ULONG_MAX);

}
