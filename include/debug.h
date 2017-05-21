#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdbool.h>

/*
 * Debug tooling
 */

#define DEBUG_LOGGING true
// Enable various log_debug statements in the code
  #define DEBUG_SCHEDULER true
  #define DEBUG_CONTEXT_SWITCH true
  #define DEBUG_KERNEL_MAIN true
  #define DEBUG_TASK true
  #define DEBUG_SYSCALL true

#define NOP do {} while(0)

#if DEBUG_MODE
/**
 * This function is made to only be a breakpoint in GDB
 * By doing 'break debugger', we can stop on any calls to this
 * while we can also stub this to otherwise be a NOP
 */
void debugger();

#include <stdarg.h>
#include <stdio.h>
/**
 * Prints with a formatted string to STDERR for debugging output
 */
#if DEBUG_LOGGING
#define log_debug(format, ...) fprintf(stderr, format "\n\r", ## __VA_ARGS__)
#else
#define log_debug(format, ...) NOP
#endif

#include <assert.h>
#else

#define debugger() NOP
#include <bwio.h>
#if DEBUG_LOGGING
#define log_debug(format, ...) bwprintf(COM2, format "\n\r", ## __VA_ARGS__)
#else
#define log_debug(format, ...) NOP
#endif
#define assert(x) NOP
#endif

#if DEBUG_SCHEDULER
#define log_scheduler_kern(format, ...) log_debug("SC  " format, ## __VA_ARGS__)
#define log_scheduler_task(format, ...) log_debug("ST  " format, ## __VA_ARGS__)
#else
#define log_scheduler_kern(format, ...) NOP
#define log_scheduler_task(format, ...) NOP
#endif

#if DEBUG_CONTEXT_SWITCH
#define log_context_swich(format, ...) log_debug("CS  " format, ## __VA_ARGS__)
#else
#define log_context_swich(format, ...) NOP
#endif

#if DEBUG_KERNEL_MAIN
#define log_kmain(format, ...) log_debug("M   " format, ## __VA_ARGS__)
#else
#define log_kmain(format, ...) NOP
#endif

#if DEBUG_TASK
#define log_task(format, tid, ...) log_debug("T%d  " format, tid, ## __VA_ARGS__)
#else
#define log_task(format, tid, ...) NOP
#endif

#if DEBUG_SYSCALL
#define log_syscall(format, ...) log_debug("SY  " format, ## __VA_ARGS__)
#else
#define log_syscall(format, ...) NOP
#endif

#endif
