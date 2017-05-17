#ifndef __SYSCALL_H__
#define __SYSCALL_H__

// for syscall_t definition
#include <kern/context.h>

void syscall_handle(kernel_request_t *arg);

void syscall_create(task_descriptor_t *task, kernel_request_t *arg);
void syscall_my_tid(task_descriptor_t *task, kernel_request_t *arg);
void syscall_my_parent_tid(task_descriptor_t *task, kernel_request_t *arg);
void syscall_pass(task_descriptor_t *task, kernel_request_t *arg);
void syscall_exit(task_descriptor_t *task, kernel_request_t *arg);
void syscall_send(task_descriptor_t *task, kernel_request_t *arg);
void syscall_receive(task_descriptor_t *task, kernel_request_t *arg);

#endif