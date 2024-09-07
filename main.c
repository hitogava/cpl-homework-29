#include "coopmult.h"

void foo(void *arg) {
    task_arg *targ = (task_arg *) arg;

    if (targ->curr-- == 0)
        curr_task()->ready = 1;
#ifdef DEBUG
    printf("curr = %d, end = %d\n", curr_task()->arg->curr, curr_task()->arg->end);
#endif
    if (curr_task()->ready)
        printf("%d\n", curr_task()->arg->end);

    coopmult_sleep(curr_task()->env);

    foo(arg);
}

#define ta_sz 5

int main(void) {
    return 0;
}
