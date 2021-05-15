
#ifndef GAME_NETWORK_H_FILE
#define GAME_NETWORK_H_FILE

#include <cstdint>

#include "net.h"
#include "game_data.h"

class GameNetwork {
protected:
  bool running;
  unsigned long last_rx_time;
  unsigned int tx_packets;
  unsigned int tx_errors;
  SPRITE *local_spr;
  SPRITE *remote_spr;
  uint16_t msg_buffer[NET_MSG_SIZE/sizeof(uint16_t)];
  
public:
  GameNetwork() { running = false; }
  void init(SPRITE *local, SPRITE *remote);
  void step();
  unsigned int get_num_tx_packets() { return tx_packets; }
  unsigned int get_num_tx_errors() { return tx_errors; }
  bool is_running() { return running; }
};

#endif /* GAME_NETWORK_H_FILE */
