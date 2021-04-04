/*
 * (c) Yoichi Tanibayashi
 */
#include "Nixie.h"

//============================================================================
// class NixieElement
//----------------------------------------------------------------------------
void NixieElement::setup(uint8_t pin) {
  this->set_pin(pin);

  this->set_blightness(0);
  this->_on = false;
}

void NixieElement::set_blightness(uint8_t blightness) {
  this->_blightness = blightness;
}
void NixieElement::inc_blightness() {
  if (this->_blightness < BLIGHTNESS_MAX) {
    this->_blightness++;
  }
}
void NixieElement::dec_blightness() {
  if (this->_blightness > 0) {
    this->_blightness--;
  }
}
uint8_t NixieElement::get_blightness() {
  return this->_blightness;
}

void NixieElement::set_pin(uint8_t pin) {
  this->_pin = pin;
}
uint8_t NixieElement::get_pin() {
  return this->_pin;
}

void NixieElement::on() {
  this->_on = true;
}
void NixieElement::off() {
  this->_on = false;
}
boolean NixieElement::is_on() {
  return this->_on;
}
//============================================================================
// class Effect
//----------------------------------------------------------------------------
Effect::Effect(effect_id_t eid, NixieElement *element) {
  this->_id = eid;
  this->_Element = element;
}
Effect::~Effect() {}

void Effect::start(unsigned long start_ms, unsigned long tick_ms) {
  this->_start_ms = start_ms;
  this->_tick_ms = tick_ms;
  (void)this->tick(this->_start_ms);
  this->_active = true;
}
void Effect::start(unsigned long start_ms, unsigned long tick_ms, int el) {
  this->start(start_ms, tick_ms);
}
void Effect::start(unsigned long start_ms, unsigned long tick_ms,
                   int el1, int el2) {
  this->start(start_ms, tick_ms);
}

void Effect::loop(unsigned long cur_ms) {
  if ( ! tick(cur_ms) ) {
    return;
  }
  Serial.println("Effect::loop()");
}

void Effect::end() {
  this->_active = false;
} // Effect::end()

effect_id_t Effect::get_id() {
  return this->_id;
} // Effect::get_id()

boolean Effect::is_active() {
  return this->_active;
} // Effect::is_active()

boolean Effect::tick(unsigned long cur_ms) {
  if ( this->_tick_ms == 0 ) {
    return false;
  }
  this->_prev_tick = this->_tick;
  this->_tick = (cur_ms - this->_start_ms) / this->_tick_ms;
  if ( this->_tick == this->_prev_tick ) {
    return false;
  }
  return true;
} // Effect::tick()

//============================================================================
// class EffectFadeIn
//----------------------------------------------------------------------------
EffectFadeIn::EffectFadeIn(NixieElement *element)
: Effect::Effect(EFFECT_FADEIN, element) {
}

void EffectFadeIn::start(unsigned long start_ms,
                    unsigned long tick_ms,
                    int element) {
  Effect::start(start_ms, tick_ms);

  this->_el = element;
  this->_Element[this->_el].set_blightness(0);
} // EffectFadeIn::start()

void EffectFadeIn::loop(unsigned long cur_ms) {
  if ( ! this->tick(cur_ms) ) {
    return;
  }

  NixieElement *e = &(this->_Element[this->_el]); // 重要！ポインタ渡し
  uint8_t bl = e->get_blightness();
  if ( bl < BLIGHTNESS_MAX ) {
    e->inc_blightness();
  } else {
    this->end();
  }
} // EffectFadeIn::loop()
//============================================================================
// class EffectFadeOut
//----------------------------------------------------------------------------
EffectFadeOut::EffectFadeOut(NixieElement *element)
: Effect::Effect(EFFECT_FADEOUT, element) {
}

void EffectFadeOut::start(unsigned long start_ms, unsigned long tick_ms,
                    int element) {
  Effect::start(start_ms, tick_ms);

  this->_el = element;
  this->_Element[this->_el].set_blightness(BLIGHTNESS_MAX);
}

void EffectFadeOut::loop(unsigned long cur_ms) {
  if ( ! this->tick(cur_ms) ) {
    return;
  }

  NixieElement *e = &(this->_Element[this->_el]);
  uint8_t bl = e->get_blightness();
  if ( bl > 0 ) {
    e->dec_blightness();
  } else {
    this->end();
  }
} // EffectFadeOut::loop()
//============================================================================
// class EffectXFade
//----------------------------------------------------------------------------
EffectXFade::EffectXFade(NixieElement *element)
: Effect::Effect(EFFECT_XFADE, element) {
}

void EffectXFade::start(unsigned long start_ms, unsigned long tick_ms,
                        int el_in, int el_out) {
  Effect::start(start_ms, tick_ms);

  this->_el_in  = el_in;
  this->_el_out = el_out;
  this->_Element[this->_el_in].set_blightness(0);
  this->_Element[this->_el_out].set_blightness(BLIGHTNESS_MAX);
} // EffectXFade::start()

void EffectXFade::loop(unsigned long cur_ms) {
  if ( ! this->tick(cur_ms) ) {
    return;
  }

  NixieElement *e_in  = &(this->_Element[this->_el_in]);
  NixieElement *e_out = &(this->_Element[this->_el_out]);
  uint8_t bl_in  = e_in->get_blightness();
  uint8_t bl_out = e_out->get_blightness();
  int end_count = 0;
  if ( bl_in < BLIGHTNESS_MAX ) {
    e_in->inc_blightness();
  } else {
    end_count++;
  }
  if ( bl_out > 0 ) {
    e_out->dec_blightness();
  } else {
    end_count++;
  }
  if ( end_count >= 2 ) {
    this->end();
  }
} // EffectXFade::loop()
//============================================================================
// class EffectShuffle
//----------------------------------------------------------------------------
EffectShuffle::EffectShuffle(NixieElement *element)
: Effect(EFFECT_SHUFFLE, element) {
}

void EffectShuffle::start(unsigned long start_ms, unsigned long tick_ms,
                          int n, int element) {
  Effect::start(start_ms, tick_ms);

  this->_n = n;
  this->_el = element;

  for (int e=0; e < NIXIE_NUM_DIGIT_N; e++) {
    this->_Element[e].set_blightness(0);
  }
} // EffectShuffle::start()

void EffectShuffle::loop(unsigned long cur_ms) {
  static int el_random = 0;

  if ( ! this->tick(cur_ms) ) {
    return;
  }

  this->_Element[el_random].set_blightness(0);

  if ( this->_tick >= this->_n ) {
    this->end();
    return;
  }
  
  el_random = random(NIXIE_NUM_DIGIT_N);
  this->_Element[el_random].set_blightness(BLIGHTNESS_MAX);
} // EffectShuffle::loop()

void EffectShuffle::end() {
  Effect::end();

  this->_Element[this->_el].set_blightness(BLIGHTNESS_MAX);
} // EffectShuffle::end()

//============================================================================
// class EffectBlink
//----------------------------------------------------------------------------
EffectBlink::EffectBlink(NixieElement *element)
: Effect(EFFECT_BLINK, element) {
} // EffectBlink::EffectBlink()

void EffectBlink::start(unsigned long start_ms, unsigned long tick_ms,
                        int el_n) {
  Effect::start(start_ms, tick_ms);

  this->_el_n = el_n;
  this->_onoff = true;

  for (int e=0; e < this->_el_n; e++) {
    this->_blightness[e] = this->_Element[e].get_blightness();
  } // for(e)
} // EffectBlink::start()

void EffectBlink::loop(unsigned long cur_ms) {
  if ( ! this->tick(cur_ms) ) {
    return;
  }

  if ( this->_onoff ) {
    this->_onoff = false;
    for (int e=0; e < this->_el_n; e++) {
      this->_Element[e].set_blightness(0);
    } // for(e)
    return;
  }

  // this->_onoff == false
  this->_onoff = true;
  for (int e=0; e < this->_el_n; e++) {
    this->_Element[e].set_blightness(this->_blightness[e]);
  } // for(e)
} // EffectBlink::loop()

void EffectBlink::end() {
  Effect::end();

  for (int e=0; e < this->_el_n; e++) {
    this->_Element[e].set_blightness(this->_blightness[e]);
  } // for(e)
} // EffectBlink::end()

//============================================================================
// class EffectRandomOnOff
//----------------------------------------------------------------------------
EffectRandomOnOff::EffectRandomOnOff(NixieElement *element)
: Effect(EFFECT_RANDOM_ONOFF, element) {

} // EffectRandomOnOff::EffectRandomOnOff()

void EffectRandomOnOff::start(unsigned long start_ms, unsigned long tick_ms,
                              int element) {
  Effect::start(start_ms, tick_ms);

  this->_el = element;
  this->_Element[this->_el].set_blightness(BLIGHTNESS_MAX);
} // EffectRandomOnOff::start()

void EffectRandomOnOff::loop(unsigned long cur_ms) {
  if ( ! this->tick(cur_ms) ) {
    return;
  }

  NixieElement *e = &(this->_Element[this->_el]); // 重要！ポインタ渡し
  if (random(10) > 5) {
    e->set_blightness(BLIGHTNESS_MAX);
  } else {
    e->set_blightness(0);    
  }
} // EffectRandomOnOff::loop()

void EffectRandomOnOff::end() {
  Effect::end();

  this->_Element[this->_el].set_blightness(BLIGHTNESS_MAX);
} // EffectRandomOnOff::end()

//============================================================================
// class NixieTube
//----------------------------------------------------------------------------
void NixieTube::setup(int element_n, uint8_t *pin) {
  this->element_n = element_n;
  this->element = new NixieElement[this->element_n];
  for (int d=0; d < element_n; d++) {
    this->element[d].setup(pin[d]);
  } // for (d)
  this->_ef = (Effect *)NULL;
} // NixieTube::setup()

void NixieTube::loop(unsigned long cur_ms) {
  if ( this->_ef == (Effect *)NULL ) {
    return;
  }
  if ( ! this->_ef->is_active() ) {
    delete this->_ef;
    this->_ef = (Effect *)NULL;
    return;
  }
  // Effect is active
  this->_ef->loop(cur_ms);
} // NixieTube::loop()

Effect *NixieTube::init_effect(effect_id_t eid) {
  if ( this->_ef != (Effect *)NULL ) {
    if ( this->_ef->is_active() ) {
      this->_ef->end();
    }
    delete this->_ef;
    this->_ef = (Effect *)NULL;
  }

  switch (eid) {
  case EFFECT_FADEIN:   return new EffectFadeIn(this->element);
  case EFFECT_FADEOUT:  return new EffectFadeOut(this->element);
  case EFFECT_XFADE:    return new EffectXFade(this->element);
  case EFFECT_SHUFFLE:  return new EffectShuffle(this->element);
  case EFFECT_BLINK:    return new EffectBlink(this->element);
  case EFFECT_RANDOM_ONOFF:    return new EffectRandomOnOff(this->element);
  default:
    Serial.println("ERROR: eid = " + String(eid));
    return (Effect *)NULL;
  }
} // NixieTube::init_effect()

void NixieTube::end_effect() {
  if ( this->_ef == (Effect *)NULL ) {
    return;
  }
  if ( this->_ef->is_active() ) {
    this->_ef->end();
  }
  delete this->_ef;
  this->_ef = (Effect *)NULL;
} // NixieTube::end_effect()

boolean NixieTube::effect_is_active() {
  return this->_ef;
} // NixieTube::effect_is_active()

void NixieTube::fadein_start(unsigned long start_ms,
                             unsigned long tick_ms,
                             int element) {
  this->_ef = this->init_effect(EFFECT_FADEIN);
  this->_ef->start(start_ms, tick_ms, element);
} // NixieTube::fadein_start()

void NixieTube::fadeout_start(unsigned long start_ms,
                              unsigned long tick_ms,
                              int element) {
  this->_ef = this->init_effect(EFFECT_FADEOUT);
  this->_ef->start(start_ms, tick_ms, element);
} // NixieTube::fadeout_start()

void NixieTube::xfade_start(unsigned long start_ms,
                            unsigned long tick_ms,
                            int el_in, int el_out) {
  this->_ef = this->init_effect(EFFECT_XFADE);
  this->_ef->start(start_ms, tick_ms, el_in, el_out);
} // NixieTube::xfade_start()

void NixieTube::shuffle_start(unsigned long start_ms, unsigned long tick_ms,
                              int n, int el) {
  this->_ef = this->init_effect(EFFECT_SHUFFLE);
  this->_ef->start(start_ms, tick_ms, n, el);
} // NixieTube::shuffle_start()

void NixieTube::blink_start(unsigned long start_ms, unsigned long tick_ms,
                            int el_n) {
  this->_ef = this->init_effect(EFFECT_BLINK);
  this->_ef->start(start_ms, tick_ms, el_n);
}

void NixieTube::randomOnOff_start(unsigned long start_ms,
                                  unsigned long tick_ms,
                                  int el) {
  this->_ef = this->init_effect(EFFECT_RANDOM_ONOFF);
  this->_ef->start(start_ms, tick_ms, el);
}
  
//============================================================================
// class NixieArray
//----------------------------------------------------------------------------
NixieArray::NixieArray(uint8_t clk, uint8_t stobe, uint8_t data, uint8_t blank,
                       uint8_t num_pin[NIXIE_NUM_N][NIXIE_NUM_DIGIT_N],
                       uint8_t colon_pin[NIXIE_COLON_N][NIXIE_COLON_DOT_N]) {
  uint8_t pout[] = {clk, stobe, data, blank};

  for (int p=0; p < sizeof(pout) / sizeof(uint8_t); p++) {
    pinMode(pout[p], OUTPUT);
    digitalWrite(pout[p], LOW);
  }
  this->_pin_clk   = clk;
  this->_pin_stobe = stobe;
  this->_pin_data  = data;
  this->_pin_blank = blank;

  for (int n=0; n < NIXIE_NUM_N; n++) {
    this->num[n].setup(NIXIE_NUM_DIGIT_N, num_pin[n]);
  } // for(n)

  for (int c=0; c < NIXIE_COLON_N; c++) {
    this->colon[c].setup(NIXIE_COLON_DOT_N, colon_pin[c]);
    for (int d=0; d < NIXIE_COLON_DOT_N; d++) {
      pinMode(colon_pin[c][d], OUTPUT);
      digitalWrite(colon_pin[c][d], LOW);
    }
  } // for(c)
} // NixieArray::setup()

void NixieArray::loop(unsigned long cur_ms) {
  for (int ti=0; ti < NIXIE_NUM_N; ti++) {
    this->num[ti].loop(cur_ms);
  } // for(ti)
  for (int ti=0; ti < NIXIE_COLON_N; ti++) {
    this->colon[ti].loop(cur_ms);
  } // for(ti)
} // NixieArray::loop()

void NixieArray::end_all_effect() {
  for (int t=0; t < NIXIE_NUM_N; t++) {
    this->num[t].end_effect();
  } // for(t)
  for (int c=0; c < NIXIE_COLON_N; c++) {
    this->colon[c].end_effect();
  } // for(c)
} // NixieArray::end_all_effect()

void NixieArray::set_onoff(unsigned long cur_ms) {
  uint8_t timing = cur_ms % BLIGHTNESS_MAX;

  // 数字部
  for (int t=0; t < NIXIE_NUM_N; t++) {
    for (int e=0; e < this->num[t].element_n; e++) {
      this->num[t].element[e].off();
      if (this->num[t].element[e].get_blightness() > timing) {
        this->num[t].element[e].on();
      }
    } // for(e)
  } // for(t)

  // colorn部
  for (int t=0; t < NIXIE_COLON_N; t++) {
    for (int e=0; e < this->colon[t].element_n; e++) {
      this->colon[t].element[e].off();
      if (this->colon[t].element[e].get_blightness() > timing) {
        this->colon[t].element[e].on();
      }
    } // for(e)
  } // for(t)
} // NixieArray::set_onoff()

void NixieArray::display(unsigned long cur_ms) {
  uint8_t pin_n = NIXIE_NUM_N * NIXIE_NUM_DIGIT_N;
  uint8_t val[pin_n];

  //--------------------------------------------------------------------------
  this->loop(cur_ms); // ニキシー管 全て
  //--------------------------------------------------------------------------
  this->set_onoff(cur_ms); // 全エレメントの表示状態更新
  //--------------------------------------------------------------------------
  // 数字部の表示処理
  for (int p=0; p < pin_n; p++) {
    val[p] = HIGH;
  }

  for (int t=0; t < NIXIE_NUM_N; t++) {
    for (int e=0; e < NIXIE_NUM_DIGIT_N; e++) {
      uint8_t pin = this->num[t].element[e].get_pin();

      if ( this->num[t].element[e].is_on() ) {
        val[pin] = LOW;
      }
    } // for(e)
  } // for(t)

  //? digitalWrite(_pin_clk, LOW);
  for (int p=(pin_n - 1); p >=0; p--) {
    digitalWrite(_pin_data, val[p]);
    digitalWrite(_pin_clk, HIGH);
    delayMicroseconds(NixieArray::DISP_DELAY_US);

    digitalWrite(_pin_clk, LOW);
    delayMicroseconds(NixieArray::DISP_DELAY_US);
  }
  
  digitalWrite(_pin_stobe, HIGH);
  delayMicroseconds(NixieArray::DISP_DELAY_US);

  digitalWrite(_pin_stobe, LOW);
  delayMicroseconds(NixieArray::DISP_DELAY_US);
  //--------------------------------------------------------------------------
  // コロンの表示処理
  for (int c=0; c < NIXIE_COLON_N; c++) {
    for (int d=0; d < NIXIE_COLON_DOT_N; d++) {
      uint8_t pin = this->colon[c].element[d].get_pin();
      if ( this->colon[c].element[d].is_on() ) {
        digitalWrite(pin, LOW);  // LOW  --> ON !
      } else {
        digitalWrite(pin, HIGH); // HIGH --> OFF !
      }
    } // for(d)
  } // for(c)
} // NixieArray::display()
//============================================================================
// Local Variables:
// Mode: arduino
// Coding: utf-8-unix
// End:
