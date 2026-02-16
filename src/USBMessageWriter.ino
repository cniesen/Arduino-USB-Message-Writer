#include "DigiKeyboard.h"
#include <avr/pgmspace.h>

#define BUTTON  0
#define LEDPIN  1
#define BUZZER  2

// ---------------- Timing ----------------
#define DOT          60
#define DASH         180
#define ELEMENT_GAP  60
#define LETTER_GAP   180
#define MORSE_DELAY  2000

const unsigned long ledOnTime  = 120;
const unsigned long ledOffTime = 120;
const unsigned long ledPause   = 800;

const unsigned long charInterval = 150;
const unsigned long beepDuration = 60;

// ---------------- Message (FLASH, not RAM!) ----------------
const char message[] PROGMEM =
"Hello Dragon";

#define MESSAGE_LENGTH (sizeof(message) - 1)

// Morse AE0S
const char* callsign[] = { ".-", ".", "-----", "..." };
#define CALLSIGN_LEN 4

// ---------------- LED State Machine ----------------
uint8_t ledState = 0;
unsigned long ledTimer = 0;

// ---------------- Typing State Machine ----------------
int16_t charIndex = -1;
unsigned long typeTimer = 0;

bool buzzerOn = false;
unsigned long beepTimer = 0;

// ---------------- Morse State Machine ----------------
enum MorseState {
  MORSE_IDLE,
  MORSE_WAIT,
  MORSE_SYMBOL_ON,
  MORSE_SYMBOL_OFF,
  MORSE_ELEMENT_GAP,
  MORSE_LETTER_GAP,
  MORSE_DONE
};

MorseState morseState = MORSE_IDLE;
uint8_t morseLetter = 0;
uint8_t morseSymbol = 0;
unsigned long morseTimer = 0;

// =====================================================
// ================== LED ==============================
// =====================================================

void updateLED(unsigned long now) {

  switch (ledState) {

    case 0:
      digitalWrite(LEDPIN, HIGH);
      if (now - ledTimer >= ledOnTime) {
        ledTimer = now;
        ledState = 1;
      }
      break;

    case 1:
      digitalWrite(LEDPIN, LOW);
      if (now - ledTimer >= ledOffTime) {
        ledTimer = now;
        ledState = 2;
      }
      break;

    case 2:
      digitalWrite(LEDPIN, HIGH);
      if (now - ledTimer >= ledOnTime) {
        ledTimer = now;
        ledState = 3;
      }
      break;

    case 3:
      digitalWrite(LEDPIN, LOW);
      if (now - ledTimer >= ledPause) {
        ledTimer = now;
        ledState = 0;
      }
      break;
  }
}

// =====================================================
// ================== Typing ===========================
// =====================================================

void startTyping() {
  charIndex = 0;
  typeTimer = millis();
}

void updateTyping(unsigned long now) {

  if (charIndex >= 0 && charIndex < MESSAGE_LENGTH) {

    if (now - typeTimer >= charInterval) {

      char c = pgm_read_byte(&message[charIndex]);
      DigiKeyboard.print(c);

      digitalWrite(BUZZER, HIGH);
      buzzerOn = true;
      beepTimer = now;

      charIndex++;
      typeTimer = now;
    }
  }
  else if (charIndex == MESSAGE_LENGTH) {

    DigiKeyboard.sendKeyStroke(KEY_ENTER);
    charIndex = -1;

    morseState = MORSE_WAIT;
    morseTimer = now;
  }

  // Turn buzzer off (short key beep)
  if (buzzerOn && (now - beepTimer >= beepDuration)) {
    digitalWrite(BUZZER, LOW);
    buzzerOn = false;
  }
}

// =====================================================
// ================== Morse ============================
// =====================================================

void updateMorse(unsigned long now) {

  switch (morseState) {

    case MORSE_IDLE:
      break;

    case MORSE_WAIT:
      if (now - morseTimer >= MORSE_DELAY) {
        DigiKeyboard.print("\n\n\n");
        morseLetter = 0;
        morseSymbol = 0;
        morseState = MORSE_SYMBOL_ON;
      }
      break;

    case MORSE_SYMBOL_ON: {
      char s = callsign[morseLetter][morseSymbol];

      DigiKeyboard.print(s);
      digitalWrite(BUZZER, HIGH);

      morseTimer = now;
      morseState = MORSE_SYMBOL_OFF;
      break;
    }

    case MORSE_SYMBOL_OFF: {
      char s = callsign[morseLetter][morseSymbol];
      unsigned long duration = (s == '.') ? DOT : DASH;

      if (now - morseTimer >= duration) {

        digitalWrite(BUZZER, LOW);
        morseTimer = now;
        morseSymbol++;

        if (callsign[morseLetter][morseSymbol] == '\0') {
          morseState = MORSE_LETTER_GAP;
        } else {
          morseState = MORSE_ELEMENT_GAP;
        }
      }
      break;
    }

    case MORSE_ELEMENT_GAP:
      if (now - morseTimer >= ELEMENT_GAP) {
          morseState = MORSE_SYMBOL_ON;
      }
      break;

    case MORSE_LETTER_GAP:
      if (now - morseTimer >= LETTER_GAP) {

        DigiKeyboard.print(" ");

        morseLetter++;
        morseSymbol = 0;

        if (morseLetter >= CALLSIGN_LEN) {
          DigiKeyboard.print("\n\n\n");
          morseState = MORSE_DONE;
        } else {
          morseState = MORSE_SYMBOL_ON;
        }
      }
      break;

    case MORSE_DONE:
      morseState = MORSE_IDLE;
      break;
  }
}

// =====================================================
// ================== Setup / Loop =====================
// =====================================================

void setup() {
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(LEDPIN, OUTPUT);
  pinMode(BUZZER, OUTPUT);
}

void loop() {

  DigiKeyboard.update();  // keep USB alive

  unsigned long now = millis();

  updateLED(now);
  updateTyping(now);
  updateMorse(now);

  if (digitalRead(BUTTON) == LOW && charIndex == -1 && morseState == MORSE_IDLE) {
    startTyping();
  }
}
