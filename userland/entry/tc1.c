#include <entry/tc1.h>
#include <basic.h>
#include <bwio.h>
#include <servers/nameserver.h>
#include <idle_task.h>
#include <servers/clock_server.h>
#include <servers/uart_tx_server.h>
#include <servers/uart_rx_server.h>
#include <interactive.h>
#include <train_controller.h>
#include <priorities.h>

void tc1_entry_task() {
  Create(PRIORITY_NAMESERVER, nameserver);
  Create(PRIORITY_CLOCK_SERVER, clock_server);
  Create(0, uart_tx);
  Create(0, uart_rx);
  Create(PRIORITY_IDLE_TASK, idle_task);

  Create(PRIORITY_TRAIN_CONTROLLER_SERVER, train_controller_server);

  Create(PRIORITY_INTERACTIVE, interactive);
}
