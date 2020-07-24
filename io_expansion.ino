

/*
Only compatible (or rather tested) with an Arduino uno. May work with other arduinos but that remains to be tested.
This file effectively makes the arduino into an io expansion module for z80 cpu (should work for other older cpus as well assuming they have at max 8 bits of address available).

For now the only function for this expansion module will be outputing a high or low to a set of 4 gpio pins.

Note all "registers" default to inputs to make it easy and safe.

*/
/*
 Important Addresses:

0b00: Nothing associated with it
0b01: Configure GPIOs for output or input, 1 = OUTPUT, 0 = input
0b10: Set/get current GPIO States , 1 = HIGH, 0 = LOW
*/

#define ADDR_PIN_COUNT 2
#define GPIO_COUNT 4

#define WRITE A0
#define READ A1
#define IO_SEL A2

const unsigned char addr_pins[] = {A3,A4};
const unsigned char data_pins[] = {2,3,4,5,6,7,8,9};

const unsigned char gpio_pins[] = {10,11,12,13};

unsigned char gpio_current_output =0x00; // stores the current output for gpios
unsigned char gpio_types = 0x00; // stores the current types for the gpio, 0 = input, 1 = output

//declarations of functions
char get_bit(unsigned int, unsigned char);
void set_type(unsigned char);
unsigned char get_type();
unsigned char get_output();
unsigned char set_state(unsigned char);
unsigned char read_addr();
void set_data_bus_io(unsigned char);
void set_data_bus(unsigned char);
unsigned char get_data_bus();

void setup()
{
	char i;
	for (i=0; i< ADDR_PIN_COUNT; i++)
		pinMode(addr_pins[i], INPUT);
	for (i=0; i< 8; i++)
		pinMode(data_pins[i], INPUT); // this may change later 
	for (i=0; i< GPIO_COUNT; i++)
		pinMode(gpio_pins[i], INPUT);
	//set the pinmodes for the ctrl signals

	pinMode(WRITE, INPUT);
	pinMode(READ, INPUT);
	pinMode(IO_SEL, INPUT);
}

void loop()
{
	char addr = read_addr();
	if (IO_SEL == 0)
	{
		switch (addr)
		{
			case 0:
			break;
			case 1:
				if (digitalRead(READ) == 0)
				{
				unsigned char states = get_type();
				set_data_bus(states);
				}
				else
				{
				unsigned char states = get_data_bus();
				set_type(states);
				}
			break;
			case 2:
				if (digitalRead(READ) == 0)
				{
				unsigned char states = get_output();
				set_data_bus(states);
				}
				else
				{
				unsigned char states = get_data_bus();
				set_state(states);
				}
			break;
			case 3:
			break;
		}
	}
	set_data_bus_io(0);
}

void set_data_bus_io(unsigned char state) // for setting input or output
{
	char i;
	for (i=0; i< 8; i++)
		pinMode(data_pins[i], state == 0 ? INPUT: OUTPUT);
}

void set_data_bus(unsigned char data)
{
	if (digitalRead(WRITE) == 0 && digitalRead(READ) == 1)
	{
		set_data_bus_io(1);
		unsigned char i;
		for (i=0; i< 8; i++)
			digitalWrite(data_pins[i], get_bit(data, i));	
	}
}

unsigned char get_data_bus()
{
	if (digitalRead(WRITE) == 1 && digitalRead(READ) == 0)
	{
		set_data_bus_io(0);
		unsigned char i;
		unsigned char data =0;
		for (i=0; i< 8; i++)
			data |= digitalRead(data_pins[i]) << i;
	}
	return 0;
}

unsigned char read_addr()
{
	unsigned char i;
	unsigned char addr = 0;
	for (i=0; i< ADDR_PIN_COUNT; i++)
		addr += digitalRead(addr_pins[i]) << i;
	return addr;
}

unsigned char get_bit(unsigned char data, unsigned char index) { return index > 7 ? 0 : ((data & (1 << index)) >> index); }

void set_type(unsigned char type)
{
	unsigned char i;
	for (i=0; i< GPIO_COUNT; i++)
	{
		gpio_types = 0;
		gpio_types |= get_bit(type, i) << i;
		pinMode(gpio_pins[i], get_bit(gpio_types,i) == 0 ? INPUT : OUTPUT);
	}
}

unsigned char get_type()
{
	return gpio_types;
}

//more gets last set output. Does not deal with if you ask for the current state of input pin. Will fix later
unsigned char get_output()
{
	return gpio_current_output;
}

unsigned char set_state(unsigned char state)
{
	unsigned char i;
	for (i=0; i< GPIO_COUNT; i++)
	{
		if (get_bit(gpio_types,i) == 1)
	 	{
			gpio_current_output =0;
			gpio_current_output |= get_bit(state, i) << i;
			digitalWrite(gpio_pins[i],get_bit(gpio_current_output,i));
		}
	}
}
