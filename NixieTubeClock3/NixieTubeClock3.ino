/*
 * (c) 2020 Yoichi Tanibayashi
 */
#include "NixieArray.h"
#include "CmdQueue.h"
#include "CmdDispatcher.h"
#include "Button.h"
#include "ModeBase.h"
#include "ModeTest1.h"

#define LOOP_DELAY_US       1 // micro seconds

#define PIN_INTR            2 // ??
#define DEBOUNCE          200 // msec

#define PIN_HV5812_CLK     26
#define PIN_HV5812_STOBE   13
#define PIN_HV5812_DATA    14
#define PIN_HV5812_BLANK    4

#define PIN_COLON_R_TOP    16
#define PIN_COLON_R_BOTTOM 16
#define PIN_COLON_L_TOP    17
#define PIN_COLON_L_BOTTOM 17

#define PIN_LED            27

#define PIN_BTN1           33
#define PIN_BTN2           34
#define PIN_BTN3           35
#define BTN_N               3
//============================================================================
uint8_t pinsIn[] = {PIN_BTN1, PIN_BTN2, PIN_BTN3};

uint8_t nixiePins[NIXIE_NUM_N][NIXIE_NUM_DIGIT_N] =
  {{ 9,  0,  1,  2,  3,  4,  5,  6,  7,  8},
   {19, 10, 11, 12, 13, 14, 15, 16, 17, 18},
   {29, 20, 21, 22, 23, 24, 25, 26, 27, 28},
   {39, 30, 31, 32, 33, 34, 35, 36, 37, 38},
   {49, 40, 41, 42, 43, 44, 45, 46, 47, 48},
   {59, 50, 51, 52, 53, 54, 55, 56, 57, 58} };

uint8_t colonPins[NIXIE_COLON_N][NIXIE_COLON_DOT_N] =
  {{PIN_COLON_R_TOP, PIN_COLON_R_BOTTOM},
   {PIN_COLON_L_TOP, PIN_COLON_L_BOTTOM} };
//----------------------------------------------------------------------------
NixieArray nixieArray;
CmdQueue cmdQ;
CmdDispatcher cmdDispatcher;
Button btnObj1, btnObj2, btnObj3;
Button *btnObj[] = {&btnObj1, &btnObj2, &btnObj3};
//----------------------------------------------------------------------------
unsigned long loopCount  = 0;
unsigned long curMsec    = 0; // msec
unsigned long prevMsec   = 0;
//----------------------------------------------------------------------------
int curTube = 0;
int curDigit = 0;
//----------------------------------------------------------------------------
#define MODE_NONE -1
#define MODE_TEST1 0
ModeTest1 modeT1;

ModeBase *Mode[] = {&modeT1};

static unsigned long MODE_N = sizeof(Mode) / sizeof(ModeBase *);

long curMode = MODE_TEST1;
long prevMode = MODE_NONE;
//============================================================================
void btn_handler() {
  static unsigned long prev_msec = 0;
  unsigned long cur_msec = millis();

  if ( cur_msec - prev_msec < DEBOUNCE ) {
    return;
  }
  prev_msec = cur_msec;

  for (int b=0; b < BTN_N; b++) {
    if ( btnObj[b]->get() ) {
      btnObj[b]->print();
    }
  }

  if ( digitalRead(PIN_BTN1) == LOW ) {
    curDigit--;
    if ( curDigit < 0 ) {
      curDigit = NIXIE_NUM_DIGIT_N - 1;
      curTube--;
      if ( curTube < 0 ) {
	curTube = NIXIE_NUM_N - 1;
      }
    }
    
    Serial.print("curTube:" + String(curTube) + " ");
    Serial.print("curDigit:" + String(curDigit) + " ");
    Serial.println();
  }

  if ( digitalRead(PIN_BTN2) == LOW) {
    Serial.print("curTube:" + String(curTube) + " ");
    Serial.print("curDigit:" + String(curDigit) + " ");

    uint8_t bl = nixieArray.get_num_blightness(curTube, curDigit);
    bl++;
    if ( bl > BLIGHTNESS_MAX ) {
      bl=0;
    }
    nixieArray.set_num_blightness(curTube, curDigit, bl);
    Serial.print("bl=" + String(bl));
    Serial.println();
  }

  if ( digitalRead(PIN_BTN3) == LOW ) {
    curDigit++;
    if ( curDigit >= NIXIE_NUM_DIGIT_N ) {
      curDigit = 0;
      curTube++;
      if ( curTube >= NIXIE_NUM_N ) {
	curTube = 0;
      }
    }
    
    Serial.print("curTube:" + String(curTube) + " ");
    Serial.print("curDigit:" + String(curDigit) + " ");
    Serial.println();
  }
} // btn_handler
//============================================================================
void setup() {
  Serial.begin(115200);
  Serial.println("begin");
  //--------------------------------------------------------------------------
  // グローバルオブジェクトの初期化
  //
  nixieArray.setup(PIN_HV5812_CLK,  PIN_HV5812_STOBE,
                   PIN_HV5812_DATA, PIN_HV5812_BLANK,
                   nixiePins, colonPins);
  cmdQ.setup();
  cmdDispatcher.setup(&nixieArray, &cmdQ);
  //--------------------------------------------------------------------------
  // 各モードの初期化
  for (int m=0; m < MODE_N; m++) {
    Mode[m]->setup(m, &nixieArray, &cmdQ);
  }
  //--------------------------------------------------------------------------
  // ボタンの初期化
  btnObj1.setup(PIN_BTN1, "BTN1");
  btnObj2.setup(PIN_BTN2, "BTN2");
  btnObj3.setup(PIN_BTN3, "BTN3");
  //--------------------------------------------------------------------------
  for (int i=0; i < sizeof(pinsIn) / sizeof(uint8_t); i++) {
    pinMode(pinsIn[i], INPUT);
    int val = digitalRead(pinsIn[i]);
    Serial.println("SW[" + String(i) + "]=" + String(val) );
  }
  uint8_t intr_pin1 = digitalPinToInterrupt(PIN_BTN1);
  uint8_t intr_pin2 = digitalPinToInterrupt(PIN_BTN2);
  uint8_t intr_pin3 = digitalPinToInterrupt(PIN_BTN3);

  Serial.println("digitalPinToInterrupt:");
  Serial.println(" " + String(PIN_BTN1) + " --> " + String(intr_pin1));
  Serial.println(" " + String(PIN_BTN2) + " --> " + String(intr_pin2));
  Serial.println(" " + String(PIN_BTN3) + " --> " + String(intr_pin3));

  attachInterrupt(intr_pin1, btn_handler, CHANGE);
  attachInterrupt(intr_pin2, btn_handler, CHANGE);
  attachInterrupt(intr_pin3, btn_handler, CHANGE);
  //--------------------------------------------------------------------------
  // セットアップ時に投入したコマンドを実行
  prevMsec = millis();
  curMsec = millis();
  cmdDispatcher.loop(curMsec);
} // setup()
//============================================================================
void loop() {
  prevMsec = curMsec;
  curMsec = millis();

  // ボタン
  for (int b=0; b < BTN_N; b++) {
    if ( btnObj[b]->get() ) {
      btnObj[b]->print();
    }
  }

  if (curMode != prevMode) {
    Mode[curMode]->init();       // モード変更時の初期化
    cmdDispatcher.loop(curMsec); // キューイングされたコマンドの実行
    prevMode = curMode;
  }

  Mode[curMode]->loop(curMsec); // コマンドは全てキューイングされる
  cmdDispatcher.loop(curMsec);  // キューイングされたコマンド実行

  nixieArray.display(curMsec);  // 表示

  loopCount++;
  delayMicroseconds(LOOP_DELAY_US);
} // loop()
//============================================================================
// Local Variables:
// Mode: arduino
// Coding: utf-8-unix
// End: