#include <basic.h>
#include <util.h>
#include <trains/switch_controller.h>
#include <servers/nameserver.h>
#include <servers/uart_tx_server.h>
#include <servers/clock_server.h>
#include <kernel.h>
#include <priorities.h>

static int switch_controller_tid = -1;

void solenoid_off() {
  Delay(20); // Delay 200 ms
  Putc(COM1, 32);
}

int switch_to_index(int sw) {
  if (sw >= 1 && sw <= 18) {
    return sw-1;
  }
  if (sw >= 153 && sw <= 156) {
    return sw = sw-134-1; // 19 -> 153, etc
  }
  return -1;
}

void switch_controller() {
  switch_controller_tid = MyTid();
  RegisterAs(NS_SWITCH_CONTROLLER);

  int requester;
  switch_control_request_t request;

  char buf[2];
  buf[0] = 0;
  buf[1] = 0;

  int solenoid_off_tid = -1;

  int switchState[NUM_SWITCHES];
  for (int i = 0; i < NUM_SWITCHES; i++) {
    switchState[i] = -1;
  }

  while (1) {
    ReceiveS(&requester, request);
    if (request.type == SWITCH_GET) {
      int index = switch_to_index(request.index);
      KASSERT(index != -1, "Asked for invalid switch %d by %d", request.index, requester);
      int state = switchState[index];
      if (switchState[index] == -1) {
        state = SWITCH_STRAIGHT;
      }
      ReplyS(requester, state);
    } else if (request.type == SWITCH_SET) {
      ReplyN(requester);
      int index = switch_to_index(request.index);
      KASSERT(index != -1, "Asked to set invalid switch %d to %d", request.index, request.value);
      if (switchState[index] != request.value) {
        switchState[index] = request.value;
        buf[1] = request.index;
        if (request.value == SWITCH_CURVED) {
          buf[0] = 34; Putcs(COM1, buf, 2);
        } else if (request.value == SWITCH_STRAIGHT) {
          buf[0] = 33; Putcs(COM1, buf, 2);
        }
        if (solenoid_off_tid != -1) {
          Destroy(solenoid_off_tid);
        }
        solenoid_off_tid = CreateRecyclable(PRIORITY_SWITCH_CONTROLLER_SOLENOIDS_OFF, solenoid_off);
        {
          int time = Time();
          uart_packet_fixed_size_t packet;
          packet.len = 6;
          packet.type = PACKET_SWITCH_DATA;
          jmemcpy(&packet.data[0], &time, sizeof(int));
          packet.data[4] = request.index;
          packet.data[5] = request.value;
          PutFixedPacket(&packet);
        }
      }
    }
  }
}

int SetSwitch(int sw, int state) {
  #if defined(DEBUG_MODE)
  // For using a Switch Controller fixture
  if (switch_controller_tid == -1) {
    switch_controller_tid = WhoIsEnsured(NS_SWITCH_CONTROLLER);
  }
  #endif
  log_task("SetSwitch switch=%d state=%d", active_task->tid, sw, state);
  KASSERT(switch_controller_tid != -1, "Switcher Controller not initialized");
  switch_control_request_t request;
  request.type = SWITCH_SET;
  request.index = sw;
  request.value = state;
  SendSN(switch_controller_tid, request);
  return 0;
}

int GetSwitchState(int sw) {
  #if defined(DEBUG_MODE)
  // For using a Switch Controller fixture
  if (switch_controller_tid == -1) {
    switch_controller_tid = WhoIsEnsured(NS_SWITCH_CONTROLLER);
  }
  #endif
  log_task("SetSwitch switch=%d state=%d", active_task->tid, sw, state);
  KASSERT(switch_controller_tid != -1, "Switcher Controller not initialized");
  switch_control_request_t request;
  request.type = SWITCH_GET;
  request.index = sw;
  int state;
  SendS(switch_controller_tid, request, state);
  return state;
}
