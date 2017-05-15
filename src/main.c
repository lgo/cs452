 /*
 * iotest.c
 */

#include <bwio.h>

void* kernelFinished = 0;
void* kernelSp = 0;
void* taskSp = 0;
int taskRunning = 1;

typedef int (*interrupt_handler)(int);

void __asm_swi_handler();
void __asm_switch_to_task(void* task_sp);
void __asm_start_task(void* task_sp, void* task_pc);

void* __syscall(void* stack, int syscall, int arg1, int arg2) {
  syscall = syscall & 0x0000FFFF;

  if (syscall == 0) {
    taskRunning = 0;
  }

  // do the syscall
  bwprintf(COM2, "__syscall: syscall %x\n\r", syscall);

  // Assume it returned 5;
  int syscall_return = syscall+1;

  // Put the syscall return value on the passed in stack
  asm(
    "stmfd %1!, {%0}\n\t"
    : /* no output */
    : "r" (syscall_return), "r" (stack)
  );
  stack -= 4;

  // save stack pointer

  // determine which task to run next
  void* taskToRunStack = stack; // For now just run the same task

  if (taskRunning) {
    return taskToRunStack;
  } else {
    return 0;
  }
}

void Exit() {
  bwprintf(COM2, "Exit\n\r");
  asm(
    "swi #0\n\t"
  );
}

asm(
"\n"
"__asm_swi_handler:\n\t"
  "stmfd sp!, {r4-r12, lr}\n\t"

  // set up args for syscall
  "mov r3, r1\n\t"
  "mov r2, r0\n\t"
  "mov r0, sp\n\t"
  "ldr r1, [lr, #-4]\n\t"

  // switch to kernel stack
  "ldr r4, .__asm_swi_handler_data+0\n\t"
  "ldr sp, [sl, r4]\n\t"

  // handle syscall
  "bl __syscall(PLT)\n\t"

  // go to __asm_finished_tasks if not given a stack pointer
  "cmp r0, #0\n\t"
  "beq __asm_finished_tasks\n\t"

  // save kernel sp
  "ldr r4, .__asm_swi_handler_data+0\n\t"
  "str sp, [sl, r4]\n\t"

  // switch to returned stack pointer
  "mov sp, r0\n\t"

  // restore registers, including an extra r0 return value added by __syscall
  "ldmfd sp!, {r0, r4-r12, lr}\n\t"
  "movs pc, lr\n\t"

"\n"
"__asm_finished_tasks:\n\t"
  // switch to kernel stack before the syscall
  "ldr r4, .__asm_swi_handler_data+0\n\t"
  "ldr sp, [sl, r4]\n\t"

  // load kernel state before tasks were finished
  "ldmfd sp!, {r4-r12, lr}\n\t"
  // continue
  "mov pc, lr\n\t"

"\n"
"__asm_start_task:\n\t"
  // save kernel lr
  "stmfd sp!, {r4-r12, lr}\n\t"

  // save kernel sp
  "ldr r4, .__asm_swi_handler_data+0\n\t"
  "str sp, [sl, r4]\n\t"

  // switch to new stack
  "mov sp, r0\n\t"

  // Store Exit function as the return place
  "ldr r4, .__asm_swi_handler_data+4\n\t"
  "ldr lr, [sl, r4]\n\t"

  "movs pc, r1\n\t"

"\n"
".__asm_swi_handler_data:\n\t"
  ".word kernelSp(GOT)\n\t"
  ".word Exit(GOT)\n\t"
);

int Create(int priority, void* code) {
  bwprintf(COM2, "Create\n\r");
  int x;
  asm ("swi #1\n\t");
  asm (
    "mov %0, r0\n\t"
    : "=r" (x)
  );
  return x;
}

int MyTid() {
  bwprintf(COM2, "MyTid\n\r");
  int x;
  asm ("swi #2\n\t");
  asm (
    "mov %0, r0\n\t"
    : "=r" (x)
  );
  return x;
}

int MyParentTid() {
  bwprintf(COM2, "MyParentTid\n\r");
  int x;
  asm ("swi #3\n\t");
  asm (
    "mov %0, r0\n\t"
    : "=r" (x)
  );
  return x;
}

int Pass() {
  bwprintf(COM2, "Pass\n\r");
  int x;
  asm ("swi #4\n\t");
  asm (
    "mov %0, r0\n\t"
    : "=r" (x)
  );
  return x;
}

void task() {
  bwprintf(COM2, "task: Hello, world!\n\r");
  int x;
  
  x = Pass();
  bwprintf(COM2, "task: syscall returned %d!\n\r\n\r", x);

  x = MyTid();
  bwprintf(COM2, "task: syscall returned %d!\n\r\n\r", x);

  x = MyParentTid();
  bwprintf(COM2, "task: syscall returned %d!\n\r\n\r", x);

  x = Create(0, 0);
  bwprintf(COM2, "task: syscall returned %d!\n\r\n\r", x);
}

int main() {
  kernelSp = 0;
  bwsetfifo(COM2, OFF);
  *((interrupt_handler*)0x28) = (interrupt_handler)&__asm_swi_handler;
  bwprintf(COM2, "main: Hello, world!\n\r");
  __asm_start_task((void*)0x00228000, &task);
  bwprintf(COM2, "main: Done!\n\r");
  return 1;
}
