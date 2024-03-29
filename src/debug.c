#include <stdint.h>
#include <util.h>
#include <basic.h>
#include <debug.h>
#include <bwio.h>
#include <jstring.h>
#include <kern/interrupts.h>
#include <kern/context.h>
#include <terminal.h>

extern int next_starting_task;
extern int last_started_task;

#define REDBOOT_ENTRYPOINT 0x218000

extern int main_fp;

/**
 * Dumps memory in hex and ASCII around mem, specifically len amount spread
 * before and after
 * @param description to show ahead of the dump
 * @param mem         to print
 * @param len         total bytes to print (NOTE: MUST BE 4 BYTE MULTIPLE)
 */
void hex_dump(char *description, char *mem, int len) {
  KASSERT(len % 4 == 0, "Can only dump groups of 4 bytes, expected len%%4==0, got len=%d", len);
  int i;
  int j;
  char *curr_mem = mem - len;
  bwprintf(COM2, "%s:\n\r", description);
  for (i = 0; i < 2*len; i += 4) {
    if (curr_mem == mem) {
      // highlight the desired address
      bwputstr(COM2, WHITE_BG BLACK_FG);
      bwprintf(COM2, "%08x", (unsigned int) curr_mem);
      bwputstr(COM2, RESET_ATTRIBUTES);
    } else {
      bwprintf(COM2, "%08x", (unsigned int) curr_mem);
    }

    bwprintf(COM2, " is %08x  ", *(unsigned int *)curr_mem);

    for (j = 0; j < 4; j++) {
      char c = curr_mem[j];
      if (is_alphanumeric(c) || c == '_') bwputc(COM2, c);
      else bwputc(COM2, '.');
    }
    bwputstr(COM2, "\n\r");
    curr_mem += 4;
  }
}

char *name_not_found = "(name not found)";

/**
 * Gets a function name, given the functions beginning program counter
 * @param  pc
 * @return    a name, or "(name not found)" if there was an issue.
 */
char *get_func_name(unsigned int *pc) {
  // check if a name is present via. the upper bits of the addr being 0xFF
  unsigned int name_length = *(pc - 1) & 0xFF;
  char *chars = (char *) (pc - 1);
  chars -= name_length;

  int i = 0;
  while (chars[i] != '\0') {
    if (!(is_alphanumeric(chars[i]) || chars[i] == '&' || chars[i] == '_')) {
      return name_not_found;
    }
    i++;
  }

  return chars;
}

#if DEBUG_MODE

void print_stack_trace(unsigned int fp, int lr) {}
void PrintAllTaskStacks(int focused_task) {}

#else

static inline uint32_t get_next_fp(uint32_t fp) {
  return VMEM(fp - 12);
}

static inline uint32_t get_lr(uint32_t fp) {
  return VMEM(fp - 4);
}

static inline uint32_t get_func_starting_pc(uint32_t fp) {
  return VMEM(fp) - 16;
}

void uart_tx_notifier();

void print_stack_trace(unsigned int fp, int lr) {
	if (!fp) return;
	unsigned int pc = 0, depth = 20;

  // Ensure the FP is in range
	if (fp <= 0x218000 || fp >= 0x2000000) {
		bwprintf(COM2, RED_BG "fp out of range: %08x" RESET_ATTRIBUTES "\n\r", fp);
		return;
	}

	do {
    // Get the PC
		pc = get_func_starting_pc(fp);

    // Get the assembly offset
		int asm_line_num = (lr == 0) ? 0 : ((lr - pc) >> 2);
    // Get the function name
    char *name = get_func_name((unsigned int *)pc);

    if (name == name_not_found) {
      hex_dump("something", (char *) fp, 16);
      bwprintf(COM2, "uart_notifier addr=%08x", (unsigned int) uart_tx_notifier);
      return;
    }

    bwprintf(COM2, "  %s @ %08x+%d  fp=%08x\n\r", name, pc, asm_line_num, fp);

    // Get next functions current execution location + FP
		lr = get_lr(fp);
		fp = get_next_fp(fp);

    // Break if end of user task stack (Exit is put into LR by scheduler)
    if (lr == (unsigned int) Exit) break;

    // Break if end of kernel stack. Redboot is in the LR, which is below the
    // redboot entry -- well probably just an arbitrarily low number
	} while (lr > REDBOOT_ENTRYPOINT);
}

void PrintAllTaskStacks(int focused_task) {
  int i;
  task_descriptor_t *task;

  for (i = 0; i < MAX_TASKS; i++) {
    task = &ctx->descriptors[i];
    if (task->state == STATE_ZOMBIE) continue;
    if (i == focused_task) continue;
    bwprintf(COM2, "Task %d (%s) stack\n\r", i, task->name);
    if (task->was_interrupted) {
      bwprintf(COM2, "  task was interrupted - backtraces not implemented\n\r");
      continue;
    }
    PrintTaskBacktrace(i);
    bwprintf(COM2, "\n\r");
  }

  if (focused_task >= 0) {
    task = &ctx->descriptors[focused_task];
    bwprintf(COM2, GREY_BG BLACK_FG "CURRENT" RESET_ATTRIBUTES " Task %d (%s) backtrace\n\r", focused_task, task->name);
    // get fp
    unsigned int fp;
    asm volatile("mov %0, fp @ save fp" : "=r" (fp));
    // skip this frame, because it's PrintAllTaskStacks
    fp = get_next_fp(fp);
    // print current task backtrace
    print_stack_trace(fp, 0);
  }
}


static inline bool is_valid_task(int tid) {
  return tid < MAX_TASKS;
}

void PrintTaskBacktrace(int tid) {
  KASSERT(is_valid_task(tid), "Can't take the backtrace of an invalid task.");

  task_descriptor_t *task = &ctx->descriptors[tid];

  // Get the FP and LR from the saved stack context, done by {swi,hwi}_handler
  unsigned int *fp_location = task->stack_pointer + (5 * 4);
  unsigned int *lr_location = task->stack_pointer + (1 * 4);
  // hex_dump("Target stack memory", ctx->descriptors[tid].stack_pointer, 8 * 7);
  // bwprintf(COM2, "Predirected fp location=%08x\n\r", (unsigned int) fp_location);
  unsigned int fp = *fp_location;
  unsigned int lr = *lr_location;
  // bwprintf(COM2, "Predirected fp=%08x\n\r", fp - 0x20);
  // hex_dump("Target frame", (void *) (fp - 0x20), 8 * 10);
  // fp - 0x20 because ??, but it worked through observation
  if (tid == 4) return;
  if (tid == 7) {
    hex_dump("Idle task zone", (char *) fp, 20 * 8);
  }

  fp -= 0x20;

  // the PC pointed to by the saved stack will be bad, as it's in the middle of a fcn
  if (task->was_interrupted) fp = get_next_fp(fp);

  print_stack_trace(fp, lr);
}

void print_stats() {
  bwputstr(COM2, "\n\r" WHITE_BG BLACK_FG "===== STATS" RESET_ATTRIBUTES "\n\r");
  if (last_started_task != -1) {
    bwprintf(COM2, "Last task %d: %s\n\r", last_started_task, ctx->descriptors[last_started_task].name);
    bwprintf(COM2, "  stack_pointer=%08x\n\r", (unsigned int) ctx->descriptors[last_started_task].stack_pointer);
  }
  if (next_starting_task != -1) {
    bwprintf(COM2, "Next task %d: %s\n\r", next_starting_task, ctx->descriptors[next_starting_task].name);
    bwprintf(COM2, "  stack_pointer=%08x\n\r", (unsigned int) ctx->descriptors[next_starting_task].stack_pointer);
  }
  bwputstr(COM2, "Execution time\n\r");
  int i;
  #if !defined(DEBUG_MODE)
  for (i = 0; i < MAX_TASKS; i++) {
    task_descriptor_t *task = &ctx->descriptors[i];
    if (task->state == STATE_ZOMBIE) continue;
    // Skip recyclable tasks
    if (task->is_recyclable) continue;
    bwprintf(COM2, " Task%s %3d:%-40s %10ums (Total) %10uus (Send) %10uus (Recv) %10uus (Repl)\n\r",
      task->state == STATE_ZOMBIE ? ":Z" : "  ",
      i, task->name,
      io_time_ms(task->execution_time),
      io_time_us(task->send_execution_time),
      io_time_us(task->recv_execution_time),
      io_time_us(task->repl_execution_time)
    );
  }
  #endif
}

void print_logs() {
  logs[log_length] = 0;
  bwputstr(COM2, "\n\r" WHITE_BG BLACK_FG "===== LOGS" RESET_ATTRIBUTES "\n\r");
  bwputstr(COM2, logs);
}

typedef void (*interrupt_handler)(void);

/**
 * This function is what returns back to redboot in a privilaged mode, otherwise
 * redboot will fail. it needs to be in SVC mode to work, so we use this as
 * an SWI handler to enter privilaged mode
 */
void __exit_kernel_svc() {
  asm volatile ("msr cpsr_c, #211");
  asm volatile ("sub sp, %0, #16" : : "r" (main_fp));
  asm volatile ("ldmfd sp, {sl, fp, sp, pc}");
}

/**
 * Cleanly exits the kernel into redboot doing nothing else.
 * In order to do this we need to be in privilaged mode, so we set the handler
 * then call into privilaged mode
 */
void exit_kernel() {
  *((interrupt_handler*)0x28) = (interrupt_handler)&__exit_kernel_svc;
  // go into __exit_kernel_svc as IRQ (privilaged)
  asm volatile ("swi #5");
}


//
// This is needed because for some reason if we run our program more than
//   once, on the second time we immediately hit the interrupt handler
//   before starting a user task, even though we should start with
//   interrupts disabled
// TODO: figure out why ^
// NOTE: others were having this issue too

/**
 * Cleans up before we exit, among that is:
 *   - Clear out any interrupt bits
 *   - Turning off the track
 *   - Setting good exit values (UART2 FIFO ON)
 *   - Logs information on the program, uptime, stats, stacks
 */
void cleanup(bool print_stacks) {

  // Make sure to clear out any pending packet data, also
  // sends a signal that the program has finished to the GUI
  for (int i = 0; i < 260; i++) {
    bwputc(COM2, 0x0);
  }

  interrupts_clear_all();

  // We have to do this a few times, otherwise it doesn't turn off
  // Smells like bad CTS?
  bwputc(COM1, 0x61);
  bwputc(COM1, 0x61);
  bwputc(COM1, 0x61);
  bwputc(COM1, 0x61);
  bwsetfifo(COM2, ON);

  print_logs();
  print_stats();

  bwputstr(COM2, "\n\r" WHITE_BG BLACK_FG "===== TASK STACKS" RESET_ATTRIBUTES "\n\r");
  bwputstr(COM2, "Bug Joey to have this implemented :'(\n\t ");

  // Right now no one prints all stacks, because this breaks on certain tasks
  // Usually that happens if they were interrupted, but also other times
  if (print_stacks) {
    if (active_task) {
      PrintAllTaskStacks(active_task->tid);
    }
    else {
      PrintAllTaskStacks(-1);
      bwprintf(COM2, "Kernel stack\n\r");
      PrintBacktrace();
    }
  }
}
#endif
