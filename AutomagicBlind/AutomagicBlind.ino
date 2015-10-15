
#define INTERRUPT_PIN0 0
#define DEBUG
#define STEP_PIN 13
#define DIR_PIN 12
#define SLEEP_PIN 11
#define RESET_PIN 9
#define JOY_PIN1 0
#define JOY_PIN2 1
#define CW 1
#define CCW 0

uint16_t curr_step = 0;
uint16_t steps_per_rev = 200;
uint16_t steps_to_down = 500;
uint8_t step_pin_state = 0;

char			serial_data[101];			// Array for incoming serial-data 
unsigned char	serial_index = 0;			// How many bytes have been received?
char			string_started = 0;			// Only saves data if string starts with right byte

enum direction {cw, ccw};
enum mode {neutral, up, down, sleep};
mode current_mode = neutral;

unsigned long curr_millis;
unsigned long curr_us;
unsigned long prev_step_us = 0;
unsigned long us_between_steps = 10000;
unsigned long prev_step_millis = 0;
unsigned long millis_between_steps = 10; // milliseconds

// Timer 1 interrupt 1Hz
/*ISR(TIMER1_COMPA_vect)
{
	
}
*/
void stepMotor(uint8_t dir)
{
	// Set the direction
	digitalWrite(DIR_PIN, dir);
	// Step motor one step
	digitalWrite(STEP_PIN, HIGH);
	digitalWrite(STEP_PIN, LOW);
}

void handleMovement()
{
	if (current_mode == up)
	{
		if (curr_step > 0)
		{
			stepMotor(CW);
			curr_step--;
		}
		else
		{
			toggleSleep();
			current_mode = sleep;
		}
	}
	else if (current_mode == down)
	{
		if (curr_step < steps_to_down)
		{
			stepMotor(CCW);
			curr_step++;
		}
		else
		{
			toggleSleep();
			current_mode = sleep;
		}
	}
}

void handleSerialCommands()
{
	if (Serial.available())
	{
		if (string_started == 1)
		{
			serial_data[serial_index++] = Serial.read();
			
			if (serial_data[serial_index - 4] == 'D' &&
				serial_data[serial_index - 3] == 'O' &&
				serial_data[serial_index - 2] == 'W' &&
				serial_data[serial_index - 1] == 'N')
			{
#ifdef DEBUG
				Serial.println("Received command DOWN.");
#endif
				if (current_mode == sleep)
					toggleSleep();
				current_mode = down;
			}

			else if (serial_data[serial_index - 2] == 'U' &&
				serial_data[serial_index - 1] == 'P')
			{
#ifdef DEBUG
				Serial.println("Received command UP.");
#endif
				if (current_mode == sleep)
					toggleSleep();
				current_mode = up;
			}
			else if (serial_data[serial_index - 4] == 'S' &&
					serial_data[serial_index - 3] == 'T' &&
					serial_data[serial_index - 2] == 'O' &&
					serial_data[serial_index - 1] == 'P')
			{
#ifdef DEBUG
				Serial.println("Received command STOP.");
#endif
				current_mode = neutral;
			}
			else if (serial_data[serial_index - 5] == 'S' &&
					serial_data[serial_index - 4] == 'L' &&
					serial_data[serial_index - 3] == 'E' &&
					serial_data[serial_index - 2] == 'E' &&
					serial_data[serial_index - 1] == 'P')
			{
#ifdef DEBUG
				Serial.println("Received command SLEEP.");
#endif
				if (current_mode != sleep)
				{
					toggleSleep();
					current_mode = sleep;
				}
				else
				{
					toggleSleep();
					current_mode = neutral;
				}
			}
			else if (serial_data[serial_index - 5] == 'D' &&
				serial_data[serial_index - 4] == 'E' &&
				serial_data[serial_index - 3] == 'B' &&
				serial_data[serial_index - 2] == 'U' &&
				serial_data[serial_index - 1] == 'G')
			{
#ifdef DEBUG
				Serial.println("Received command DEBUG.");
#endif
				printDebug();
			}
		}
		else if (Serial.read() == '$')
		{
			string_started = 1;
		}
	}
}

void toggleSleep()
{
	uint8_t res = digitalRead(SLEEP_PIN);

	if (res == LOW)
	{
		digitalWrite(SLEEP_PIN, HIGH);
	}
	else
	{
		digitalWrite(SLEEP_PIN, LOW);
	}
}

void handleJoyStick()
{
	uint16_t value = analogRead(JOY_PIN1);
	Serial.print("value:");
	Serial.println(value);
	if ((value > 712) && (curr_step > 0))
	{
		stepMotor(CW);
		curr_step--;
	}
	else if ((value < 312) && (curr_step < steps_to_down))
	{
		stepMotor(CCW);
		curr_step++;
	}
}

void setup()
{
	Serial.begin(19200);
	pinMode(STEP_PIN, OUTPUT);
	pinMode(DIR_PIN, OUTPUT);
	pinMode(SLEEP_PIN, OUTPUT);
	pinMode(RESET_PIN, OUTPUT);
	/*
	cli();		// disable all interrupts
	// Setup 1 Hz interrupt on timer 1
	TCCR1A = 0;								// set TCCR1A to 0
	TCCR1B = 0;								// set TCCR1B to 0
	TCNT1 = 0;								// initialize counter value to 0
	OCR1A = 15624;							// set compare match register for 1hz increments (16,000,000 / (1024 * 1))-1
	TCCR1B |= (1 << WGM12);					// turn on CTC mode
	TCCR1B |= (1 << CS12) | (1 << CS10);	// Set CS10 and CS12 bits for 1024 prescaler
	TIMSK1 |= (1 << OCIE1A);				// enable timer compare interrupt
	sei();		// enable all interrupts
	*/
	digitalWrite(RESET_PIN, HIGH);
	digitalWrite(SLEEP_PIN, HIGH);
	digitalWrite(DIR_PIN, HIGH);

	for (int i = 0; i <= 10; i++)
	{
		digitalWrite(STEP_PIN, HIGH);
		digitalWrite(STEP_PIN, LOW);
		delay(10);
	}
	delay(100);
	digitalWrite(DIR_PIN, LOW);
	for (int i = 0; i <= 10; i++)
	{
		digitalWrite(STEP_PIN, HIGH);
		digitalWrite(STEP_PIN, LOW);
		delay(10);
	}
}

void loop()
{
	handleSerialCommands();
	
	curr_millis = millis();
	if (curr_millis - prev_step_millis >= millis_between_steps)
	{
		prev_step_millis += millis_between_steps;
		handleJoyStick();
		handleMovement();
	}
	/*
	if (curr_millis - prev_step_millis >= millis_between_steps)
	{
		prev_step_millis += millis_between_steps;

		switch (current_mode)
		{
		case neutral:
			break;
		case up:
			if (curr_step > 0)
			{
				stepMotor(CW);
				curr_step--;
			}
			else
			{
				current_mode = neutral;
			}
			break;
		case down:
			if (curr_step < steps_to_down)
			{
				stepMotor(CCW);
				curr_step++;
			}
			else
			{
				current_mode = neutral;
			}
			break;
		case sleep:
			break;
		default:
			break;
		}
	}*/
}

void printDebug()
{
	Serial.println("----DEBUG----");
	Serial.print("curr_step: ");
	Serial.println(curr_step);
	Serial.print("steps_to_down: ");
	Serial.println(steps_to_down);
	Serial.print("current_mode: ");
	switch (current_mode)
	{
	case neutral:
		Serial.println("neutral");
		break;
	case up:
		Serial.println("up");
		break;
	case down:
		Serial.println("down");
		break;
	case sleep:
		Serial.println("sleep");
		break;
	default:
		Serial.println("mode not known");
		break;
	}
}
