#include <DS3232RTC.h>
#include <Time.h> 
#include <Wire.h> 
#include "FastLED.h"
#include <BH1750.h>

#define NUM_LEDS 30 // Number of LED controles (remember I have 3 leds / controller)
#define COLOR_ORDER RGB  // Define color order for your strip
#define BTN_HOURS_PIN 2
#define BTN_MINUTES_PIN 3
#define BTN_BRIGHTNESS_PIN 4
#define BTN_COLOR_PIN 5
#define DATA_PIN 6  // Data pin for led comunication
#define BRIGHTNESS_THRESHOLD 150
#define BRIGHTNESS_LOW 5
#define BRIGHTNESS_FULL 255
#define BLACK 0x000000
#define FRAMES_PER_SECOND  24
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

CRGB leds[NUM_LEDS]; // Define LEDs strip
byte digits[10][7] = { {0,1,1,1,1,1,1},  // Digit 0
					 {0,1,0,0,0,0,1},   // Digit 1
					 {1,1,1,0,1,1,0},   // Digit 2
					 {1,1,1,0,0,1,1},   // Digit 3
					 {1,1,0,1,0,0,1},   // Digit 4
					 {1,0,1,1,0,1,1},   // Digit 5
					 {1,0,1,1,1,1,1},   // Digit 6
					 {0,1,1,0,0,0,1},   // Digit 7
					 {1,1,1,1,1,1,1},   // Digit 8
					 {1,1,1,1,0,1,1} };  // Digit 9 | 2D Array for numbers on 7 segment
int digitOffset[4] = { 23, 16, 7, 0 };
bool dotIsVisible = true;
int timeChanged = 0;
uint16_t brightness = 0;
uint8_t gHue = 0; // rotating "base color" used by many of the patterns
BH1750 lightMeter;

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void(*SimplePatternList[])();
SimplePatternList gPatterns = { rainbow };
uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current

void setup(){
	Serial.begin(9600);
	Wire.begin();
	lightMeter.begin();
	delay(3000); // 3 second delay for recovery
	LEDS.addLeds<WS2811, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);; // Set LED strip type
	LEDS.setBrightness(BRIGHTNESS_FULL); // Set initial brightness
	pinMode(BTN_HOURS_PIN, INPUT_PULLUP); // Define Color Mode
	pinMode(BTN_BRIGHTNESS_PIN, INPUT_PULLUP); // Define Hours adjust button pin
	pinMode(BTN_COLOR_PIN, INPUT_PULLUP); // Define Minutes adjust button pin
	pinMode(BTN_MINUTES_PIN, INPUT_PULLUP); // Defines Minutes adjust button pin
}

void nextPattern()
{
	Serial.println("Next pattern");
	gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE(gPatterns);
}

void rainbow()
{
	fill_rainbow(leds, NUM_LEDS, gHue, 7);
}

int GetTime() {
	tmElements_t Now;
	RTC.read(Now);
	printTime(Now);
	dotIsVisible = (Now.Second % 2 == 0);
	return (Now.Hour * 100 + Now.Minute);
};

// Check Light sensor and set brightness accordingly
void BrightnessCheck() {
	uint16_t sensorValue = lightMeter.readLightLevel();
	uint16_t difference = sensorValue - brightness;
	difference = abs(difference);
	if (difference > 10)
	{
		Serial.print("Brightness Changed from "); Serial.print(brightness); Serial.print(" to "); Serial.println(sensorValue);
		brightness = sensorValue;
		(brightness < BRIGHTNESS_THRESHOLD) ? LEDS.setBrightness(BRIGHTNESS_FULL) : LEDS.setBrightness(BRIGHTNESS_LOW);
	}
};

void TimeToLEDArray() {
	int Now = GetTime();
	int cursor = NUM_LEDS;
	CHSV color = CHSV(gHue, 255, 255);

	dotIsVisible ? (leds[14] = color) : (leds[14] = BLACK);
	dotIsVisible ? (leds[15] = color) : (leds[15] = BLACK);

	for (int i = 0; i < 4; i++)
	{
		int digit = Now % 10; // get last digit in time
		int cursor = digitOffset[i];
		for (int k = 0; k <= 6; k++) {
			if (digits[digit][k] == 1) { leds[cursor] = color; }
			else if (digits[digit][k] == 0) { leds[cursor] = BLACK; };
			cursor++;
		};
		Now /= 10;
	}
};

void printTime(tmElements_t time)
{
	if (time.Second != timeChanged)
	{
		timeChanged = time.Second;
		Serial.print("Time: ");
		print2Digits(time.Hour);
		Serial.print(":");
		print2Digits(time.Minute);
		Serial.print(":");
		print2Digits(time.Second);
		Serial.println("");
	}
}

void print2Digits(byte value)
{
	if (value < 10)
	{
		Serial.print("0");
	}
	Serial.print(value);
}

void TimeAdjust() {
	int btnHours = digitalRead(BTN_HOURS_PIN);
	int btnMinutes = digitalRead(BTN_MINUTES_PIN);
	int btnColorMode = digitalRead(BTN_COLOR_PIN);

	if (btnHours == LOW || btnMinutes == LOW || btnColorMode == LOW)
	{
		delay(500);
		tmElements_t Now;
		RTC.read(Now);
		int hour = Now.Hour;
		int minutes = Now.Minute;
		int second = Now.Second;
		if (btnHours == LOW) {
			Now.Hour == 23 ? Now.Hour = 0 : Now.Hour++;
		}
		else if(btnMinutes == LOW) {
			Now.Minute == 59 ? Now.Minute = 0 : Now.Minute++;
		}
		else if (btnColorMode == LOW)
		{
			nextPattern();
		};

		printTime(Now);
		RTC.write(Now);
	}
}
void loop()  // Main loop
{
	BrightnessCheck(); // Check brightness
	TimeAdjust(); // Check to se if time is geting modified
	TimeToLEDArray(); // Get leds array with required configuration
	// send the 'leds' array out to the actual LED strip
	// Call the current pattern function once, updating the 'leds' array
	//gPatterns[gCurrentPatternNumber]();
	FastLED.show();
	gHue++;
	// insert a delay to keep the framerate modest
	FastLED.delay(1000 / FRAMES_PER_SECOND);
	//EVERY_N_MILLISECONDS(20) { gHue++; } // slowly cycle the "base color" through the rainbow
}