#include "game_network.h"

#include "net.h"
#include "util.h"

void GameNetwork::init(SPRITE *local, SPRITE *remote)
{
  // ESP.getFreeHeap()
  printf("-> initializing network\n");
  if (net_init() != 0) {
    printf("Error initializing network\n");
    running = 0;
    return;
  }
  running = 1;
  tx_errors = 0;

  local_spr = local;
  remote_spr = remote;
}

void GameNetwork::step()
{
  if (! running) {
    return;
  }

  if (net_can_send_message()) {
    uint16_t *p = msg_buffer;
    *p++ = local_spr->x;
    *p++ = local_spr->y;
    *p++ = local_spr->frame;
    if (net_send_message((uint8_t *) msg_buffer) != 0) {
      tx_errors++;
    }
  }

  while (net_message_available()) {
    if (net_read_message((uint8_t *) msg_buffer) != 0) {
      break;
    }
    uint16_t *p = msg_buffer;
    remote_spr->x = (short) *p++;
    remote_spr->y = (short) *p++;
    remote_spr->frame = *p++;
    last_rx_time = millis();
  }
}
