/* Program to display instructions as well as statistics
 * Encoders are LOW when blocked
 */

#include <LiquidCrystal.h>

#define ENCODER0_PIN 2
#define ENCODER1_PIN 3
#define NEXT_PIN 7
#define PREV_PIN 4

#define MAX_LINES 10

LiquidCrystal lcd(13, 12, 11, 10, 9, 8);

// can only display 32 characters at a time
//            16|
const char* str[MAX_LINES] = {
"Load the rope on\
the spool.     ",
"Tie the lightest\
weight to rope.",
"Turn crank, feel\
effort required",
"Tie next weight \
and repeat.    ",
"You should feel \
an increasing  ",
"amount of energy\
required as    ",
"you work with   \
heavier weights",
"Change crank and\
turn it        ",
"observe change  \
in effort.     ",
"x denotes crank \
length.        "
};

bool display_state = 0;
int display_pos = 0;
byte instruction_pos;
bool shouldUpdateDisplay;
volatile int rot_pos = 0; // Count of disc holes passed
volatile bool encoderA = 0;
volatile bool encoderB = 0;
volatile bool rot_dir = 0; // Clockwise is 0

void display_text(int step) {
	const char* currentStr = str[step];
	lcd.setCursor(0, 0);
	if (strlen(currentStr) > 15) {
		for (int i = 0; i < 32 && currentStr[i] != '\0'; i++) {
			if (i == 16) lcd.setCursor(0, 1);
			lcd.print(currentStr[i]);
		}
	}
}
/* Intended display
 * Effort:0 cm
 * Load  :0 cm
 */
void display_stats() {
	// only gets rotation of shaft, not rotation of crank
	double d_effort = rot_pos * 0.8 * PI / 12; // 12 holes, diameter is 8mm
	// TODO: Verify formula
	double d_load = d_effort * 15 / 8 / 2; // 1:2 gear ratio to 15mm winch
	lcd.setCursor(7, 0);
	lcd.print(d_effort, 1);
	lcd.print('x');
	lcd.setCursor(7, 1);
	lcd.print(d_load, 1);
}

/* 2 encoders, second one out of phase (quadrature encoding)
 * Check rotation direction
 * At rising edge,
 * A != B -> Clockwise
 * A == B -> Anti-Clockwise
 */

void encoder_isr() {
	encoderA = digitalRead(ENCODER0_PIN);
	encoderB = digitalRead(ENCODER1_PIN);
	// check at rising edge
	rot_dir = encoderA == encoderB; // 1 when anti-clockwise
	rot_pos += 1 - 2 * rot_dir; // i.e. +1 if counter-clockwise, -1 if clockwise
}

void setup() {
	lcd.begin(16, 2);
	pinMode(NEXT_PIN, INPUT_PULLUP);
	pinMode(PREV_PIN, INPUT_PULLUP);
  pinMode(5, OUTPUT);
  analogWrite(5, 70);

	shouldUpdateDisplay = 1;
	instruction_pos = 0;

	// Run interrupt for every change in state for first encoder
	attachInterrupt(digitalPinToInterrupt(ENCODER0_PIN), encoder_isr, RISING);
	Serial.begin(9600);
}

void loop() {
	// display each step until the end where statistics will be shown
	if (instruction_pos < MAX_LINES && shouldUpdateDisplay) {
		display_text(instruction_pos);
		shouldUpdateDisplay = 0;
	}
	else if (instruction_pos == MAX_LINES) {
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("Effort:      cm");
		lcd.setCursor(0, 1);
		lcd.print("Load  :      cm");
		instruction_pos++;
	}
	else if (instruction_pos > MAX_LINES) {
		display_stats();
	}

	// because of pullup, buttons are active low
	if (!digitalRead(NEXT_PIN)) {
		if (instruction_pos != MAX_LINES)
			instruction_pos++;
		shouldUpdateDisplay = 1;
		delay(500); // delay for debouncing
	}
	else if (!digitalRead(PREV_PIN)) {
		if (instruction_pos == MAX_LINES + 1)
			instruction_pos--;
		if (instruction_pos != 0)
			instruction_pos--;
		shouldUpdateDisplay = 1;
		delay(500);
	}
}
