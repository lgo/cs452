#include <basic.h>
#include <bwio.h>
#include <kern/arm/swi_handler.h>
#include <kern/context.h>
#include <kern/context_switch.h>
#include <kern/scheduler.h>
#include <kern/task_descriptor.h>

void child_task();


void* kernelSp = 0;

typedef int (*interrupt_handler)(int);

void context_switch_init() {
  kernelSp = 0;
  *((interrupt_handler*)0x28) = (interrupt_handler)&__asm_swi_handler;
}

int context_switch_in(syscall_t call_no, int arg1, void *arg2);
void __asm_swi_handler();
void __asm_switch_to_task(void* task_sp);
void __asm_start_task(void* task_sp, void* task_pc);

asm (
  "\n"
  // export of the function
  ".global __asm_start_task\n"
  ".global __asm_switch_to_task\n"

  "__asm_switch_to_task:\n\t"

  // save kernel state and lr
  "stmfd sp!, {r4-r12, lr}\n\t"

  // save kernel sp
  "ldr r4, .__asm_swi_handler_data+0\n\t"
  "str sp, [sl, r4]\n\t"

  // switch to argument stack pointer
  "movs sp, r0\n\t"

  // recover task registers, including return argument we added
  "ldmfd sp!, {r0, r4-r12, lr}\n\t"

  // FIXME: recover task spsr


  "msr spsr_c, #208\n\t"
  "mrs r0, spsr\n\t" // store cpsr for debugging

  // begin execution of task
  "movs pc, lr\n\t"

  "__asm_swi_handler:\n\t"
  // store task state and lr
  "stmfd sp!, {r4-r12, lr}\n\t"

  // FIXME: save task spsr

  // set up args for syscall
  "mov r3, r2\n\t" // sycall arg2
  "mov r2, r1\n\t" // sycall arg1
  "mov r1, r0\n\t" // syscall number
  "mov r0, sp\n\t" // task stack pointer

  "mrs r3, cpsr\n\t" // store cpsr for debugging

  // switch to kernel stack
  "ldr r4, .__asm_swi_handler_data+0\n\t"
  "ldr sp, [sl, r4]\n\t"

  // handle syscall
  "bl __syscall(PLT)\n\t"

  // switch to kernel stack before the syscall
  "ldr r4, .__asm_swi_handler_data+0\n\t"
  "ldr sp, [sl, r4]\n\t"

  // load kernel state before tasks were finished
  "ldmfd sp!, {r4-r12, lr}\n\t"
  // return from scheduler_activate_task in kernel
  "mov pc, lr\n\t"

  "\n"
  "__asm_start_task:\n\t"
  // save kernel state and lr
  "stmfd sp!, {r4-r12, lr}\n\t"

  // save kernel sp
  "ldr r4, .__asm_swi_handler_data+0\n\t"
  "str sp, [sl, r4]\n\t"

  // switch to new stack
  "mov sp, r0\n\t"

  // Store Exit function as the return place
  "ldr r4, .__asm_swi_handler_data+4\n\t"
  "ldr lr, [sl, r4]\n\t"

  // FIXME: set task spsr to user mode
  "msr spsr_c, #208\n\t"

  "movs pc, r1\n\t"

  "\n"
  ".__asm_swi_handler_data:\n\t"
  ".word kernelSp(GOT)\n\t"
  ".word Exit(GOT)\n\t"
  );

void* __syscall(void* stack, syscall_t syscall_no, int arg1, void *arg2) {

  log_debug("CS  cpsr[4:0]=%d\n\r", (long long) arg2 & 255);
  arg2 = &child_task;

  // do the syscall
  int syscall_return = context_switch_in(syscall_no, arg1, arg2);
  log_debug("CS  syscall ret=%d\n\r", syscall_return);

  // Put the syscall return value on the passed in stack
  // note: this has to happen first before saving the stack pointer, as this is the return value
  asm (
    "stmfd %1!, {%0}\n\t"
    : /* no output */
    : "r" (syscall_return), "r" (stack)
    );
  stack -= 4;

  // determine which task to run next
  // task_descriptor_t *next_task = scheduler_next_task();
  // // save stack pointer
  // // we need to know how to get the current task
  active_task->stack_pointer = stack;
  // void* taskToRunStack = next_task->stack_pointer; // For now just run the same task

  return 0;
}

void *context_switch(syscall_t call_no, int arg1, void *arg2) {
  // NOTE: we pass the syscall number via. r0 as it's the first argument
  asm ("swi #0\n\t");
}

int context_switch_in(syscall_t call_no, int arg1, void *arg2) {
  int ret_val = 0;

  switch (call_no) {
  case SYSCALL_MY_TID:
    log_debug("CS  syscall=MyTid ret=%d\n\r", active_task->tid);
    ret_val = active_task->tid;
    scheduler_requeue_task(active_task);
    break;
  case SYSCALL_MY_PARENT_TID:
    log_debug("CS  syscall=MyParentTid ret=%d\n\r", active_task->parent_tid);
    ret_val = active_task->parent_tid;
    scheduler_requeue_task(active_task);
    break;
  case SYSCALL_CREATE:
    log_debug("CS  syscall=Create\n\r");
    task_descriptor_t *new_task = td_create(ctx, active_task->tid, arg1, arg2);
    log_debug("CS  new task priority=%d tid=%d\n\r", arg1, new_task->tid);
    scheduler_requeue_task(new_task);
    scheduler_requeue_task(active_task);
    ret_val = new_task->tid;
    break;
  case SYSCALL_PASS:
    log_debug("CS  syscall=Pass\n\r");
    scheduler_requeue_task(active_task);
    break;
  case SYSCALL_EXIT:
    log_debug("CS  syscall=Exit\n\r");
    scheduler_exit_task();
    break;
  default:
    break;
  }

  log_debug("CS  queue size=%d\n\r", scheduler_ready_queue_size());

  // Reschedule the world
  // scheduler_reschedule_the_world();

  return ret_val;
}
