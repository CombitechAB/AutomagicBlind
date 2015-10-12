
#define INTERRUPT_PIN0 0
#define DEBUG
#define STEP_PIN 13
#define DIR_PIN 12
#define CW 0
#define CCW 1

uint16_t curr_revolutions = 0;
uint16_t steps_per_rev = 200;
uint16_t steps_to_down = 10000;
uint8_t step_pin_state = 0;

char			serial_data[101];			// Array for incoming serial-data 
unsigned char	serial_index = 0;			// How many bytes have been received?
char			string_started = 0;			// Only saves data if string starts with right byte

enum direction {cw, ccw};
enum mode {neutral, up, down};
mode current_mode = neutral;

unsigned long curr_millis;
unsigned long prev_step_millis = 0;
unsigned long millis_between_steps = 1; // milliseconds

// Timer 1 interrupt 1Hz
ISR(TIMER1_COMPA_vect)
{
	
}

void stepMotor(uint8_t dir)
{
	// Set the direction
	if (dir == CW)
	{
		digitalWrite(DIR_PIN, LOW);
	}
	else
	{
		digitalWrite(DIR_PIN, HIGH);		
	}

	// Step motor one step
	digitalWrite(STEP_PIN, HIGH);
	digitalWrite(STEP_PIN, LOW);
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
				current_mode = down;
			}

			else if (serial_data[serial_index - 2] == 'U' &&
				serial_data[serial_index - 1] == 'P')
			{
#ifdef DEBUG
				Serial.println("Received command UP.");
#endif
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
		}
		else if (Serial.read() == '$')
		{
			string_started = 1;
		}
	}
}

void setup()
{
	Serial.begin(19200);
	pinMode(STEP_PIN, OUTPUT);
	pinMode(DIR_PIN, OUTPUT);
	
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
	
}

void loop()
{
	handleSerialCommands();

	curr_millis = millis();
	
	if (curr_millis - prev_step_millis >= millis_between_steps)
	{
		prev_step_millis += millis_between_steps;

		switch (current_mode)
		{
		case neutral:
			break;
		case up:
			if (curr_revolutions > 0)
			{
				stepMotor(CW);
				curr_revolutions--;
			}
			else
			{
				current_mode = neutral;
			}
			break;
		case down:
			if (curr_revolutions < steps_to_down)
			{
				stepMotor(CCW);
				curr_revolutions++;
			}
			else
			{
				current_mode = neutral;
			}
			break;
		default:
			break;
		}
	}
}
