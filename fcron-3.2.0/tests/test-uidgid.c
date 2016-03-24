#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

// if run as user and suid fcron sgid fcron: can set(e)uid() from/to user/fcron back and forth
// if run as root and suid fcron sgid fcron: can seteuid() from/to root/fcron back and forth BUT CANNOT setuid back to root
// if run as user and suid root sgid root: can seteuid() from/to root/fcron back and forth BUT CANNOT setuid back to root
// if run as root and suid root sgid root: can set(e)uid() from/to user/fcron back and forth (i.e. always stay uid/euid 0!)


int main(int argc, char *argv[]) {
    uid_t uid = getuid();
    uid_t euid = geteuid();
    gid_t gid = getgid();
    gid_t egid = getegid();

    printf("=== uid=%d gid=%d euid=%d egid=%d\n", getuid(), getgid(), geteuid(), getegid());

   printf("seteuid(uid)\n");
    if ( seteuid(uid) < 0 ) return 1;
    printf("=== uid=%d gid=%d euid=%d egid=%d\n", getuid(), getgid(), geteuid(), getegid());

    printf("seteuid(euid)\n");
    if ( seteuid(euid) < 0 ) return 1;
    printf("=== uid=%d gid=%d euid=%d egid=%d\n", getuid(), getgid(), geteuid(), getegid());

    printf("seteuid(uid)\n");
    if ( seteuid(uid) < 0 ) return 1;
    printf("=== uid=%d gid=%d euid=%d egid=%d\n", getuid(), getgid(), geteuid(), getegid());

    printf("seteuid(euid)\n");
    if ( seteuid(euid) < 0 ) return 1;
    printf("=== uid=%d gid=%d euid=%d egid=%d\n", getuid(), getgid(), geteuid(), getegid());

    printf("setuid(uid)\n");
    if ( setuid(uid) < 0 ) return 1;
    printf("=== uid=%d gid=%d euid=%d egid=%d\n", getuid(), getgid(), geteuid(), getegid());

    printf("setuid(euid)\n");
    if ( setuid(euid) < 0 ) return 1;
    printf("=== uid=%d gid=%d euid=%d egid=%d\n", getuid(), getgid(), geteuid(), getegid());

    printf("setuid(uid)\n");
    if ( setuid(uid) < 0 ) return 1;
    printf("=== uid=%d gid=%d euid=%d egid=%d\n", getuid(), getgid(), geteuid(), getegid());

    printf("setuid(euid)\n");
    if ( setuid(euid) < 0 ) return 1;
    printf("=== uid=%d gid=%d euid=%d egid=%d\n", getuid(), getgid(), geteuid(), getegid());


    printf("=== DONE\n");

    return 0;

}

