#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>

#define STACK_THRESHOLD 32768

typedef struct {
    void *stack_data;
    size_t stack_sz;
} stack;

struct task;

typedef struct task_arg {
    struct task *task;
    int is_ready, i, n;
} task_arg;

typedef struct task {
    void (*task_entry)(void *);
    task_arg task_args;

    int started;

    jmp_buf task_env;
    void *task_stack;

    struct task *next;
} task;

typedef struct {
    jmp_buf run_env;
    jmp_buf g_env;
    task *head_task;
    task *curr_task;
    void *g_stack;
    int busy;
} global_cntxt;

void coopmult_sleep(jmp_buf env);
void coopmult_add_task(void (*entry)(void *), void *args);
void coopmult_run();
void coopmult_switch_context();
void coopmult_save_context();
void coopmult_task_startup(void);

global_cntxt gcntxt = {0};

// #define DEBUG

void logger(const char *s) {
#ifdef DEBUG
    puts(s);
#endif
}

void foo(void *arg) {
    task_arg *targ = (task_arg *) arg;
    if (targ->i-- == 0)
        targ->is_ready = 1;
#ifdef DEBUG
    printf("n = %d, i = %d\n", targ->n, targ->i);
#endif
    if (targ->is_ready)
        printf("%d\n", targ->n);

    coopmult_sleep(targ->task->task_env);

    foo(arg);
}

void coopmult_task_startup(void) {
    gcntxt.curr_task->started = 1;
    gcntxt.curr_task->task_entry(&gcntxt.curr_task->task_args);
}

void coopmult_add_task(void (*entry)(void *), void *args) {
    task *t = malloc(sizeof(task));

    t->task_entry = entry;
    t->task_stack = (char *) malloc(STACK_THRESHOLD) + (STACK_THRESHOLD - 1);
    if (!t->task_stack) {
        fprintf(stderr, "Task stack allocation error\n");
        return;
    }

    t->started = t->task_args.is_ready = 0;
    // t->task_args.is_ready == (t->task_args.n == 0);
    gcntxt.busy += 1;

    if (args) {
        t->task_args = *((task_arg *) args);
        t->task_args.task = t;
        t->task_args.i = t->task_args.n;
    }

    if (!gcntxt.head_task) {
        t->next = NULL;
    } else {
        t->next = gcntxt.head_task;
    }
    gcntxt.head_task = t;
    gcntxt.curr_task = gcntxt.head_task;
}

void coopmult_run() {
    logger("coopmult_run");

    setjmp(gcntxt.run_env);

    if (!gcntxt.curr_task->next) // tail
        gcntxt.curr_task = gcntxt.head_task;
    else
        gcntxt.curr_task = gcntxt.curr_task->next;

    if (!gcntxt.curr_task->started) { // starting new task
#ifndef __x86_64__
        __asm__ volatile("mov %0, %%esp\n"
                         "call *%1"
                         :
                         : "r"(gcntxt.curr_task->task_stack),
                           "r"(coopmult_task_startup));

#else
        __asm__ volatile("mov %0, %%rsp\n"
                         "call *%1"
                         :
                         : "r"(gcntxt.curr_task->task_stack),
                           "r"(coopmult_task_startup));
#endif
    } else if (!gcntxt.curr_task->task_args.is_ready) {
        longjmp(gcntxt.curr_task->task_env, 1);
    } else if (gcntxt.busy == 0) {
        longjmp(gcntxt.g_env, 1);
    } else {
        longjmp(gcntxt.run_env, 1);
    }

    // if (gcntxt.busy == 0)
    //     longjmp(gcntxt.g_env, 1);
}

void coopmult_sleep(jmp_buf env) {
    logger("coopmult_sleep: enter");
    if (gcntxt.curr_task->task_args.is_ready)
        gcntxt.busy--;
    if (!setjmp(env)) {
        longjmp(gcntxt.run_env, 1);
    }
    return;
}

int main() {
    int nums[100];
    size_t i = 0;
    char ch;

    while (2 == scanf("%d%c", &nums[i++], &ch)) {
        if (nums[i - 1] == 0) {
            // i--;
            break;
        }
    }

    for (size_t j = 0; j < i; j++) {
        task_arg targ1 = {.n = nums[j]};
        coopmult_add_task(foo, &targ1);
    }
    if (!setjmp(gcntxt.g_env)) {
        coopmult_run();
    }

    // task_arg targ1 = {.n = 3, .is_ready = 0};
    // task_arg targ2 = {.n = 5, .is_ready = 0};
    // task_arg targ3 = {.n = 4, .is_ready = 0};
    // coopmult_add_task(foo, &targ1);
    // coopmult_add_task(foo, &targ2);
    // coopmult_add_task(foo, &targ3);
    // if (!setjmp(gcntxt.g_env)) {
    //     coopmult_run();
    // } else {
    //     logger("Finish");
    // }

    return 0;
}
