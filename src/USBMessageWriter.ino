#include "DigiKeyboard.h"

#define BUTTON 0
#define LEDPIN 1
#define BUZZER 2

// ---------- LED Double Blink ----------
unsigned long ledTimer = 0;
byte ledState = 0;

const unsigned long ledOnTime = 120;
const unsigned long ledOffTime = 120;
const unsigned long ledPause = 800;

// ---------- Typing Control ----------
const char message[] = "Hello World!";
int charIndex = -1;  // -1 = idle

unsigned long typeTimer = 0;
const unsigned long charInterval = 250;

unsigned long beepTimer = 0;
bool buzzerOn = false;
const unsigned long beepDuration = 60;

void updateLED() {
	unsigned long now = millis();

	switch (ledState) {
		case 0:  // ON 1
			digitalWrite(LEDPIN, HIGH);
			if (now - ledTimer >= ledOnTime) {
				ledTimer = now;
				ledState = 1;
			}
			break;

		case 1:  // OFF 1
			digitalWrite(LEDPIN, LOW);
			if (now - ledTimer >= ledOffTime) {
				ledTimer = now;
				ledState = 2;
			}
			break;

		case 2:  // ON 2
			digitalWrite(LEDPIN, HIGH);
			if (now - ledTimer >= ledOnTime) {
				ledTimer = now;
				ledState = 3;
			}
			break;

		case 3:  // OFF pause
			digitalWrite(LEDPIN, LOW);
			if (now - ledTimer >= ledPause) {
				ledTimer = now;
				ledState = 0;
			}
			break;
	}
}

void startTyping() {
	charIndex = 0;
	typeTimer = millis();
}

void updateTyping() {
	unsigned long now = millis();

	if (charIndex >= 0 && charIndex < sizeof(message) - 1) {
		if (now - typeTimer >= charInterval) {

			DigiKeyboard.print(message[charIndex]);

			// turn buzzer ON (active buzzer just HIGH)
			digitalWrite(BUZZER, HIGH);
			buzzerOn = true;
			beepTimer = now;

			charIndex++;
			typeTimer = now;
		}
	} else if (charIndex == sizeof(message) - 1) {
		DigiKeyboard.sendKeyStroke(KEY_ENTER);
		charIndex = -1;  // done
	}

	// turn buzzer OFF after short time
	if (buzzerOn && (now - beepTimer >= beepDuration)) {
		digitalWrite(BUZZER, LOW);
		buzzerOn = false;
	}
}

void setup() {
	pinMode(BUTTON, INPUT_PULLUP);
	pinMode(LEDPIN, OUTPUT);
	pinMode(BUZZER, OUTPUT);
}

void loop() {
	DigiKeyboard.update();  // keep USB alive

	updateLED();     // ALWAYS blinking
	updateTyping();  // handle typing state

	// start typing on button press (only if idle)
	if (digitalRead(BUTTON) == LOW && charIndex == -1) {
		startTyping();
	}
}
