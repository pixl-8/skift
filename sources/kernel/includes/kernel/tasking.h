#pragma once

#include <stdbool.h>

#include "types.h"
#include "utils.h"

#include "ds/list.h"

#include "kernel/paging.h"

#define PROCNAME_SIZE 128
#define STACK_SIZE 4096

typedef int THREAD;
typedef int PROCESS;

typedef u32 esp_t;
typedef void *(*thread_entry_t)(void *);

typedef enum
{
    PROCESS_RUNNING,
    PROCESS_CANCELED,
} process_state_t;

typedef enum
{
    THREAD_RUNNING,
    THREAD_SLEEP,

    THREAD_WAIT_THREAD,
    THREAD_WAIT_PROCESS,
    
    THREAD_CANCELED,
} thread_state_t;

typedef struct
{
    int id;
    char name[PROCNAME_SIZE];
    bool user;
    list_t *threads;
    page_directorie_t *pdir;

    process_state_t state;
    int exitcode;
    int waithandle;
} process_t;

typedef struct
{
    int id;
    process_t *process;
    thread_entry_t entry;

    uint esp;
    void *stack;

    thread_state_t state;
    void * exitvalue;
    int waithandle;
} thread_t;

void tasking_setup();

/* --- Thread managment ----------------------------------------------------- */

THREAD thread_self();

THREAD thread_create(PROCESS p, thread_entry_t entry, void *arg, int flags);
int thread_cancel(THREAD t);
void thread_exit(void *retval);
void *thread_wait(THREAD t);
void thread_sleep(int time);

/* --- Process managment ---------------------------------------------------- */

PROCESS process_self();

PROCESS process_create(const char *name, int user);

PROCESS process_exec(const char *filename, int argc, char **argv);

void process_cancel(PROCESS p);
void process_exit(int code);
int process_wait(PROCESS p);
void process_sleep(int time);

// Process memory managment
int process_map(PROCESS p, uint addr, uint count);
int process_unmap(PROCESS p, uint addr, uint count);