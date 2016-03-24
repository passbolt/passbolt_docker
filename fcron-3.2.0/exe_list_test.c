#include "exe_list.h"
#include "global.h"
#include "fcron.h"

/* required by log.c */
char debug_opt = 1;
char *prog_name = NULL;
char foreground = 1;
pid_t daemon_pid = 0;
uid_t rootuid = 0;

void
print_cur(exe_t * e)
{
    printf("Current entry's shell command: %s\n",
           e ? e->e_line->cl_shell : "NULL");
}

void
print_list(exe_list_t * list)
{
    exe_t *e = NULL;
    printf("Current list:\n");
    for (e = exe_list_first(list); e != NULL; e = exe_list_next(list))
        printf("  Shell command: %s\n", e ? e->e_line->cl_shell : "NULL");
}

int
main(int argc, char *argv[])
{
    exe_list_t *list = NULL;
    exe_t *e = NULL;
    cl_t l1, l2, l3, l4, l5, l6, l7;

    l1.cl_shell = "line 1";
    l2.cl_shell = "line 2";
    l3.cl_shell = "line 3";
    l4.cl_shell = "line 4";
    l5.cl_shell = "line 5";
    l6.cl_shell = "line 6";
    l7.cl_shell = "line 7";

    list = exe_list_init();

    /* trigger a resize of the list during an iteration */
    printf("Adding l1...\n");
    exe_list_add_line(list, &l1);
    printf("Adding l2...\n");
    exe_list_add_line(list, &l2);
    printf("Adding l3...\n");
    exe_list_add_line(list, &l3);
    e = exe_list_first(list);
    print_cur(e);
    e = exe_list_next(list);
    print_cur(e);
    printf("Adding l4...\n");
    exe_list_add_line(list, &l4);
    printf("Adding l5...\n");
    exe_list_add_line(list, &l5);
    printf("Adding l6...\n");
    exe_list_add_line(list, &l6);
    printf("Adding l7...\n");
    exe_list_add_line(list, &l7);

    e = exe_list_next(list);
    print_cur(e);
    e = exe_list_next(list);
    print_cur(e);
    e = exe_list_next(list);
    print_cur(e);
    e = exe_list_next(list);
    print_cur(e);
    e = exe_list_next(list);
    print_cur(e);
    e = exe_list_next(list);
    print_cur(e);

    /* "standard" iteration: */
    print_list(list);

    /* remove item at the beginning and middle of the list + add an item which is already in there */
    printf("removing l1, l3, adding l2\n");
    e = exe_list_first(list);
    print_cur(e);
    printf("Removing l1...\n");
    exe_list_remove_cur(list);
    e = exe_list_next(list);
    print_cur(e);               /* this one will be the item replacing l1 */
    e = exe_list_next(list);
    print_cur(e);
    e = exe_list_next(list);
    print_cur(e);
    printf("Removing l3...\n");
    exe_list_remove_cur(list);
    exe_list_end_iteration(list);
    printf("Adding l2...\n");
    exe_list_add_line(list, &l2);       /* we suppose the list array won't be reallocated() */

    print_list(list);

    /* remove an item at the end of the list: */
    printf("removing l5, l2\n");
    e = exe_list_first(list);
    print_cur(e);
    e = exe_list_next(list);
    print_cur(e);
    e = exe_list_next(list);
    print_cur(e);
    e = exe_list_next(list);
    print_cur(e);
    e = exe_list_next(list);
    print_cur(e);
    printf("Removing l5...\n");
    exe_list_remove_cur(list);
    exe_list_end_iteration(list);
    e = exe_list_first(list);
    print_cur(e);
    e = exe_list_next(list);
    print_cur(e);
    printf("Removing l2...\n");
    exe_list_remove_cur(list);
    exe_list_end_iteration(list);

    print_list(list);

    printf("Adding l1...\n");
    exe_list_add_line(list, &l1);

    print_list(list);

    printf("empty the list\n");
    e = exe_list_first(list);
    print_cur(e);
    exe_list_remove_cur(list);
    e = exe_list_next(list);
    print_cur(e);
    exe_list_remove_cur(list);
    e = exe_list_next(list);
    print_cur(e);
    exe_list_remove_cur(list);
    e = exe_list_next(list);
    print_cur(e);
    exe_list_remove_cur(list);
    e = exe_list_next(list);
    print_cur(e);
    exe_list_remove_cur(list);
    e = exe_list_next(list);
    print_cur(e);

    print_list(list);

    printf("Destroying the list...\n");
    list = exe_list_destroy(list);

    return 0;

}
