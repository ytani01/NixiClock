/*
 * (c) 2020 Yoichi Tanibayashi
 */
#include <Arduino.h>
#include "Cmd.h"
//============================================================================
Cmd::Cmd(NixieArray *nxa, cmd_t cmd, param_t param[CMD_PARAM_N]) {
  this->_nxa = nxa;
  this->_cmd = cmd;
  for (int p; p < CMD_PARAM_N; p++) {
    this->_param[p] = param[p];
  }
  this->_start_ms = 0;
}

void Cmd::start(unsigned long start_ms) {
  this->_start_ms = start_ms;
}
void Cmd::end() {
  //Serial.println("Cmd::end()");
  this->_cmd = CMD_NULL;
}
void Cmd::loop(unsigned long cur_ms) {
  if ( this->is_working() ) {
    Serial.println("!? Cmd[" + String(this->_cmd, HEX)
                   + "::loop(" + String(cur_ms) + ")");
    delay(1000);
  }
}

cmd_t Cmd::get_cmd() {
  return this->_cmd;
}
param_t *Cmd::get_param() {
  return this->_param;
}
param_t Cmd::get_param(int param_i) {
  return this->_param[param_i];
}

boolean Cmd::is_null() {
  return this->_cmd == CMD_NULL;
}
boolean Cmd::is_working() {
  return this->_cmd != CMD_NULL;
}

String Cmd::toString() {
  String str = "";
  str += String(this->_cmd, HEX) + ":";
  for (int p=0; p < CMD_PARAM_N; p++) {
    str += String(this->_param[p]) + " ";
  }
  return str;
}
//============================================================================
CmdSleep::CmdSleep(NixieArray *nxa, cmd_t cmd,
                   param_t param[CMD_PARAM_N]) :
  Cmd::Cmd(nxa, cmd, param) {
}

void CmdSleep::start(unsigned long start_ms) {
  Cmd::start(start_ms);

  this->_interval_ms = (unsigned long)this->get_param(0);
}

void CmdSleep::loop(unsigned long cur_ms) {
  if ( cur_ms - this->_start_ms >= this->_interval_ms ) {
    this->end();
  } else {
    delay(CmdSleep::SLEEP_DELAY);
  }
}
//============================================================================
CmdSetDigit::CmdSetDigit(NixieArray *nxa, cmd_t cmd,
                         param_t param[CMD_PARAM_N]) :
  Cmd::Cmd(nxa, cmd, param) {
}

void CmdSetDigit::start(unsigned long start_ms) {
  Cmd::start(start_ms);

  uint8_t num_i = (uint8_t)this->get_param(0);
  uint8_t digit_i = (uint8_t)this->get_param(1);
  uint8_t blightness = (uint8_t)this->get_param(2);

  this->_nxa->set_num_blightness(num_i, digit_i, blightness);

  this->end();
}
//============================================================================
CmdFadeIn::CmdFadeIn(NixieArray *nxa, cmd_t cmd,
                         param_t param[CMD_PARAM_N]) :
  Cmd::Cmd(nxa, cmd, param) {
}

void CmdFadeIn::start(unsigned long start_ms) {
  Cmd::start(start_ms);

  this->_num_i = (uint8_t)this->get_param(0);
  this->_digit_i = (uint8_t)this->get_param(1);
  this->_delay_ms = (unsigned long)this->get_param(2);

  this->_blightness = 0;
  this->_prev_ms = start_ms;

  this->_nxa->set_num_blightness(this->_num_i, this->_digit_i,
                                 this->_blightness);
}

void CmdFadeIn::loop(unsigned long cur_ms) {
  boolean endflag = false;
  
  if (cur_ms - this->_prev_ms >= this->_delay_ms) {
    if (this->_blightness >= BLIGHTNESS_MAX) {
      this->_blightness = 0;
      endflag = true;
    } else {
      this->_blightness++;
    }
    this->_nxa->set_num_blightness(this->_num_i, this->_digit_i,
                                   this->_blightness);
    if ( endflag ) {
      this->end();
      return;
    }
    this->_prev_ms = cur_ms;
  }
}
//============================================================================
CmdFadeOut::CmdFadeOut(NixieArray *nxa, cmd_t cmd,
                         param_t param[CMD_PARAM_N]) :
  Cmd::Cmd(nxa, cmd, param) {
}

void CmdFadeOut::start(unsigned long start_ms) {
  Cmd::start(start_ms);

  this->_num_i = (uint8_t)this->get_param(0);
  this->_digit_i = (uint8_t)this->get_param(1);
  this->_delay_ms = (unsigned long)this->get_param(2);

  this->_blightness = BLIGHTNESS_MAX;
  this->_prev_ms = start_ms;

  this->_nxa->set_num_blightness(this->_num_i, this->_digit_i,
                                 this->_blightness);
}

void CmdFadeOut::loop(unsigned long cur_ms) {
  if ((cur_ms - this->_prev_ms) >= this->_delay_ms) {
    /*
    Serial.println("cur_ms=" + String(cur_ms) + ","
                   + "prev_ms=" + String(this->_prev_ms) + ","
                   + "delay_ms=" + String(this->_delay_ms));
    */
    this->_blightness--;
    this->_nxa->set_num_blightness(this->_num_i, this->_digit_i,
                                   this->_blightness);
    if ( this->_blightness <= 0 ) {
      this->end();
      return;
    }
    this->_prev_ms = cur_ms;
  }
}
//============================================================================
CmdFogIn::CmdFogIn(NixieArray *nxa, cmd_t cmd,
                         param_t param[CMD_PARAM_N]) :
  Cmd::Cmd(nxa, cmd, param) {
}

void CmdFogIn::start(unsigned long start_ms) {
  Cmd::start(start_ms);

  this->_num_i = (uint8_t)this->get_param(0);
  this->_delay_ms = (unsigned long)this->get_param(1);

  this->_nxa->get_num_blightness(this->_num_i, this->_blightness);

  String str = "blightness={";
  for (int d=0; d < NIXIE_NUM_DIGIT_N; d++) {
    str += String(this->_blightness[d]) + " ";
  } // for(d)
  str += "}";
  Serial.println(str);
  
  this->_prev_ms = start_ms;
}

void CmdFogIn::loop(unsigned long cur_ms) {
  int full_count = 0;
  
  if (cur_ms - this->_prev_ms >= this->_delay_ms) {
    this->_nxa->get_num_blightness(this->_num_i, this->_blightness);

    for (int d=0; d < NIXIE_NUM_DIGIT_N; d++) {
      this->_blightness[d]++;
      if (this->_blightness[d] >= BLIGHTNESS_MAX) {
        this->_blightness[d] = BLIGHTNESS_MAX;
        full_count++;
      }
    } // for(d)

    this->_nxa->set_num_blightness(this->_num_i, this->_blightness);
    if ( full_count >= BLIGHTNESS_MAX ) {
      this->end();
      return;
    }
    this->_prev_ms = cur_ms;
  }
}
//============================================================================
CmdFogOut::CmdFogOut(NixieArray *nxa, cmd_t cmd,
                         param_t param[CMD_PARAM_N]) :
  Cmd::Cmd(nxa, cmd, param) {
}

void CmdFogOut::start(unsigned long start_ms) {
  Cmd::start(start_ms);

  this->_num_i = (uint8_t)this->get_param(0);
  this->_digit_i = (uint8_t)this->get_param(1);
  this->_delay_ms = (unsigned long)this->get_param(2);

  for (int d=0; d < NIXIE_NUM_DIGIT_N; d++) {
    this->_blightness[d] = BLIGHTNESS_MAX;
  }

  String str = "CmdfogOut::start>blightness={";
  for (int d=0; d < NIXIE_NUM_DIGIT_N; d++) {
    str += String(this->_blightness[d]) + " ";
  } // for(d)
  str += "}";
  Serial.println(str);
  
  this->_nxa->set_num_blightness(this->_num_i, this->_blightness);
  this->_prev_ms = start_ms;
}

void CmdFogOut::loop(unsigned long cur_ms) {
  int zero_count = 0;

  if (cur_ms - this->_prev_ms >= this->_delay_ms) {
    this->_nxa->get_num_blightness(this->_num_i, this->_blightness);
    
    for (int d=0; d < NIXIE_NUM_DIGIT_N; d++) {
      if (d == this->_digit_i) {
        this->_blightness[d] = BLIGHTNESS_MAX;
      } else {
        this->_blightness[d]--;
        if (this->_blightness[d] > BLIGHTNESS_MAX) { // unsigned > 0
          this->_blightness[d] = 0;
          zero_count++;
        }
      }
    } // for(d);

    this->_nxa->set_num_blightness(this->_num_i, this->_blightness);
    if ( zero_count >= BLIGHTNESS_MAX ) {
      this->end();
      return;
    }
    this->_prev_ms = cur_ms;
  }
}
//============================================================================
// Local Variables:
// Mode: arduino
// Coding: utf-8-unix
// End:
