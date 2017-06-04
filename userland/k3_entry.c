#include <basic.h>
#include <bwio.h>
#include <k3_entry.h>
#include <kernel.h>
#include <nameserver.h>
#include <clock_server.h>
#include <idle_task.h>


typedef struct {
  int delay_ticks;
  int delay_amount;
} client_data_t;

void k3_client_task() {
  volatile client_data_t data;
  int my_tid = MyTid();
  int parent_tid = MyParentTid();

  Send(parent_tid, NULL, 0, &data, sizeof(data));
  int ticks = data.delay_ticks;
  int amount = data.delay_amount;

  int clock_server_tid = WhoIs(CLOCK_SERVER);
  int i;
  for (i = 0; i < amount; i++) {
    Delay(clock_server_tid, ticks);
    bwprintf(COM2, "tid=%d delay=%d completed_delays=%d\n\r", my_tid, ticks, i+1);
  }
}

void k3_entry_task() {
  Create(1, &nameserver);
  Create(2, &clock_server);
  Create(IDLE_TASK_PRIORITY, &idle_task);

  client_data_t data;
  int recv_tid;

  // Client
  // priority = 3
  // delay_ticks = 10
  // delay_amount = 20
  Create(3, &k3_client_task);
  data.delay_ticks = 10;
  data.delay_amount = 20;
  ReceiveN(&recv_tid);
  ReplyS(recv_tid, data);

  // Client
  // priority = 4
  // delay_ticks = 23
  // delay_amount = 9
  Create(4, &k3_client_task);
  data.delay_ticks = 23;
  data.delay_amount = 9;
  ReceiveN(&recv_tid);
  ReplyS(recv_tid, data);

  // Client
  // priority = 5
  // delay_ticks = 33
  // delay_amount = 6
  Create(5, &k3_client_task);
  data.delay_ticks = 33;
  data.delay_amount = 6;
  ReceiveN(&recv_tid);
  ReplyS(recv_tid, data);

  // Client
  // priority = 6
  // delay_ticks = 71
  // delay_amount = 3
  Create(6, &k3_client_task);
  data.delay_ticks = 71;
  data.delay_amount = 3;
  ReceiveN(&recv_tid);
  ReplyS(recv_tid, data);
}

// origin/terrance/hardware-interrupts
// kept just because, this testing base might be useful
//
// #include <basic.h>
// #include <bwio.h>
// #include <k2_entry.h>
// #include <nameserver.h>
// #include <kernel.h>
// #include <ts7200.h>
//
// void k3_entry_task() {
//   bwprintf(COM2, "k3_entry\n\r");
//   // First things first, make the nameserver
//   Create(1, &nameserver);
//
//   volatile int volatile *flags = (int *)( UART2_BASE + UART_FLAG_OFFSET );
//   volatile int volatile *data = (int *)( UART2_BASE + UART_DATA_OFFSET );
//
//   while (!( *flags & RXFF_MASK ));
//   char x = *data;
// }