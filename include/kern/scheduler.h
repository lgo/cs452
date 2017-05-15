#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include <kern/task_descriptor.h>

/*
 * functionality for scheduling and swapping the active task as
 * part of scheduling
 */

/**
 * Initializes scheduler state
 * - currently global variables for x86 :(
 */
void scheduler_init();

/**
 * Exits and finishes execution of a task
 * also releases control to the kernel
 */
void scheduler_exit_task();

/**
 * Yields execution of the current task, releasing control to the kernel
 */
void scheduler_reschedule_the_world();

/**
 * Entry for execution of a new task
 * NOTE: if this function completes, the task is done and so it returns
 * control to the kernel
 * @param  td the task descriptor that is now executing
 */
void *scheduler_start_task(void *td);

/**
 * Activates a task, either beginning it or resuming it. this removes control of the kernel to the task
 * This is part of swapping for the scheduler
 *
 * @param task to start executing
 */
void scheduler_activate_task(task_descriptor_t *task);

#endif