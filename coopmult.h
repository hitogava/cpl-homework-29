#pragma once
#include "include/vector.h"

#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define STACK_SZ 4096
// #define DEBUG

typedef struct {
    int curr, end;
} task_arg;

typedef struct {
    void (*task_entry)(void *);
    task_arg *arg;
    jmp_buf env;
    void *stack;
    int started, ready;
} task;

typedef struct {
    jmp_buf run_env;
    jmp_buf main_env;
    vector *tasks;
    size_t curr, init_len;
} global_cntxt;

task *curr_task();
void advance_task();

extern global_cntxt gcntxt;
void global_cntxt_init();
void global_cntxt_destroy();

void task_init(task *task, void (*entry)(void *), void *args);
void print_tasks();

void coopmult_sleep(jmp_buf env);
void coopmult_add_task(void (*entry)(void *), void *args);
void coopmult_run();
void coopmult_task_startup(void);
