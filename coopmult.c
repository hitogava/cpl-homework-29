#include "coopmult.h"

#include <setjmp.h>

global_cntxt gcntxt;

void logger(const char *s) {
#ifdef DEBUG
    puts(s);
#endif
}

void global_cntxt_init() {
    gcntxt.tasks = malloc(sizeof(vector));
    if (!gcntxt.tasks) {
        fprintf(stderr, "global_cntxt_init: heap allocation error\n");
        return;
    }
    vector_init(gcntxt.tasks, sizeof(task), 0);
    gcntxt.curr = 0;
}

void global_cntxt_destroy() {
    vector_destroy(gcntxt.tasks);
    free(gcntxt.tasks);
}

task *curr_task() {
    return vector_get(gcntxt.tasks, gcntxt.curr);
}

void advance_task() {
    gcntxt.curr = (gcntxt.curr + 1) % gcntxt.tasks->len;
}

void print_tasks() {
    for (size_t i = 0; i < gcntxt.tasks->len; i++) {
        task *t = vector_get(gcntxt.tasks, i);
        assert(t);
        printf("Task with curr=%d, end=%d\n", t->arg->curr, t->arg->end);
    }
}

void task_init(task *task, void (*entry)(void *), void *args) {
    assert(task);
    assert(args);

    task->task_entry = entry;
    task->ready = task->started = 0;
    task->stack = malloc(STACK_SZ);
    if (!gcntxt.tasks) {
        fprintf(stderr, "task_init: heap allocation error\n");
        return;
    }
    task->arg = ((task_arg *) args);
    task->arg->curr = task->arg->end;
}

void coopmult_add_task(void (*entry)(void *), void *args) {
    logger("coopmult_add_task: enter");
    task t;
    task_init(&t, entry, args);
    vector_push_back(gcntxt.tasks, &t);
    gcntxt.init_len++;
}

void coopmult_sleep(jmp_buf env) {
    logger("coopmult_sleep: enter");
    if (!setjmp(env))
        longjmp(gcntxt.run_env, 1);
    return;
}

void coopmult_task_startup() {
    logger("coopmult_startup: enter");
    curr_task()->started = 1;
    curr_task()->task_entry(curr_task()->arg);
}

void coopmult_run() {
    logger("coopmult_run: enter");

    setjmp(gcntxt.run_env);

    if (gcntxt.init_len == 0)
        return;

    if (curr_task()->ready) { // delete from tasks
#ifdef DEBUG
        printf("DELETING TASK WITH N: %d, LEN IS %zu\n", curr_task()->arg->end, gcntxt.tasks->len);
        printf("curr_task is %zu\n", gcntxt.curr);
#endif
        task *last = vector_get(gcntxt.tasks, gcntxt.tasks->len - 1);
        memcpy(curr_task(), last, sizeof(task));
        vector_rmv_last(gcntxt.tasks);
    }

    if (gcntxt.tasks->len == 0) // exit
        longjmp(gcntxt.main_env, 1);

    advance_task();

    if (!curr_task()->started) {
        intptr_t new_sp = (intptr_t) ((char *) curr_task()->stack + STACK_SZ - 1) & -16;
#ifndef __x86_64__
        __asm__ volatile("mov %0, %%esp\n"
                         "call *%1"
                         :
                         : "r"(new_sp),
                           "r"(coopmult_task_startup));

#else
        __asm__ volatile("mov %0, %%rsp\n"
                         "call *%1"
                         :
                         : "r"(new_sp),
                           "r"(coopmult_task_startup));
#endif
    }
    longjmp(curr_task()->env, 1);
}
