/*
 *
 * ring buffer: CmdEnt[]
 *
 *            _head          _tail
 *             |              |
 *             v              v
 *         +--+--+--+--+--+--+--+--+--+--+--+--+--+
 * CmdEnt  |  |  |  |  |  |  |  |  |  |  |  |  |  |
 *         +--+--+--+--+--+--+--+--+--+--+--+--+--+
 *          0  1  2  3  4  ...                  (CMD_Q_POOL_SIZE-1)
 */
#ifndef CMD_QUEUE_H
#define CMD_QUEUE_H
#include <Arduino.h>
#include <string.h>
#include "Cmd.h"

#define CMD_Q_POOL_SIZE 256

struct CmdEnt {
  uint8_t cmd;
  uint8_t param[CMD_PARAM_N];
};

class CmdQueue {
 public:
  CmdQueue() {};

  void setup();
  
  boolean put(uint8_t cmd, uint8_t param[CMD_PARAM_N]);
  boolean get(uint8_t *cmd, uint8_t param[CMD_PARAM_N]);

  void print_all();
  
 private:
  CmdEnt _ent[CMD_Q_POOL_SIZE];
  int _head = 0;
  int _tail = 0;
}; // class CmdQueue

#endif // CMD_QUEUE_H
