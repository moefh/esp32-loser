#include "game_network.h"

#include "net.h"
#include "util.h"

#define GAME_NETWORK_MESSAGE_MAGIC1 0x1234
#define GAME_NETWORK_MESSAGE_MAGIC2 0x5678

void GameNetwork::init(SPRITE *local, SPRITE *remote)
{
  printf("-> initializing network\n");
  if (net_init() != 0) {
    printf("Error initializing network\n");
    running = false;
    return;
  }
  running = true;
  tx_errors = 0;
  tx_packets = 0;

  local_spr = local;
  remote_spr = remote;
}

void GameNetwork::step()
{
  if (! running) {
    return;
  }

  // send message if clear
  if (net_can_send_message()) {
    uint16_t *p = msg_buffer;

    // add magic numbers to header
    *p++ = GAME_NETWORK_MESSAGE_MAGIC1;
    *p++ = GAME_NETWORK_MESSAGE_MAGIC2;
    
    // save space for number of sprites
    uint16_t *num_sprites_msg = p++;

    // add player info
    *p++ = local_spr->x;
    *p++ = local_spr->y;
    *p++ = local_spr->frame;

    // add shots
    int num_shots = 0;
    for (int i = GAME_NUM_SPRITE_FIRST_LOCAL_SHOT; i < GAME_NUM_SPRITE_FIRST_REMOTE_SHOT; i++) {
      if (! game_sprites[i].def) continue;
      *p++ = game_sprites[i].x;
      *p++ = game_sprites[i].y;
      *p++ = game_sprites[i].frame;
      num_shots++;
    }

    // set number of sprites (player + shots)
    *num_sprites_msg = 1+num_shots;
    
    if (net_send_message((uint8_t *) msg_buffer) != 0) {
      tx_errors++;
    } else {
      tx_packets++;
    }
  }

  // receive message
  while (net_message_available()) {
    if (net_read_message((uint8_t *) msg_buffer) != 0) {
      break;
    }
    uint16_t *p = msg_buffer;

    // check magic numbers in header
    if (*p++ != GAME_NETWORK_MESSAGE_MAGIC1) continue;
    if (*p++ != GAME_NETWORK_MESSAGE_MAGIC2) continue;

    // read num sprites
    int num_shots = *p++ - 1;

    // read remote player
    remote_spr->x = (short) *p++;
    remote_spr->y = (short) *p++;
    remote_spr->frame = *p++;

    // read remote shots
    for (int i = 0; i < num_shots; i++) {
      game_sprites[GAME_NUM_SPRITE_FIRST_REMOTE_SHOT+i].x = *p++;
      game_sprites[GAME_NUM_SPRITE_FIRST_REMOTE_SHOT+i].y = *p++;
      game_sprites[GAME_NUM_SPRITE_FIRST_REMOTE_SHOT+i].frame = *p++;
      game_sprites[GAME_NUM_SPRITE_FIRST_REMOTE_SHOT+i].def = &game_sprite_defs[GAME_NUM_SPRITE_DEF_SHOT];
    }
    // remove absent shots
    for (int i = num_shots; i < GAME_NUM_SPRITES; i++) {
      game_sprites[GAME_NUM_SPRITE_FIRST_REMOTE_SHOT+i].def = nullptr;
    }
    
    last_rx_time = millis();
  }
}
