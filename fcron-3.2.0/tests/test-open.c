#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

int main(int argc, char *argv[]) {
    int fd;
    if ( (fd = open("/tmp/test", O_RDWR | O_CREAT, (mode_t) 0)) < 0 )
        perror("open()");

    return 0;
}
