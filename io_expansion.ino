

/*
Only compatible (or rather tested) with an Arduino uno. May work with other arduinos but that remains to be tested.
This file effectively makes the arduino into an io expansion module for z80 cpu (should work for other older cpus as well assuming they have at max 8 bits of address available).

For now the only function for this expansion module will be outputing a high or low to a set of 4 gpio pins.

Note all GPIOS default to inputs to make it easy and safe.
*/

//GPIO Registers:
#define NOP_REG 0x00 // Nothing atm
#define IO_TYPE_REG 0x01 // I/O type (Out or In) register 
#define IO_VALS_REG 0x02 // Setting or getting GPIO states (HIGH or LOW)

#define ADDR_PIN_COUNT 2
#define GPIO_COUNT 4

#define WRITE A0
#define READ A2
#define IO_SEL 2

const unsigned char addr_pins[] = {A3,A4};
const unsigned char data_pins[] = {A5,3,4,5,6,7,8,9};


const unsigned char gpio_pins[] = {10,11,12,13};

unsigned char gpio_current_output =0x00; // stores the current output for gpios
unsigned char gpio_types = 0x00; // stores the current types for the gpio, 0 = input, 1 = output

//declarations of functions
char get_bit(unsigned int, unsigned char);
void set_type(unsigned char);
unsigned char get_type();
unsigned char get_output();
void set_output(unsigned char);
unsigned char read_addr();
void set_data_bus_io(unsigned char);
void set_data_bus(unsigned char);
unsigned char get_data_bus();

void setup()
{
	Serial.begin(9600);
	char i;
	for (i=0; i< ADDR_PIN_COUNT; i++)
		pinMode(addr_pins[i], INPUT);
	for (i=0; i< 8; i++)
		pinMode(data_pins[i], INPUT); // this may change later 
	for (i=0; i< GPIO_COUNT; i++)
		pinMode(gpio_pins[i], OUTPUT);

	//set the pinmodes for the ctrl signals
	pinMode(WRITE, INPUT);
	pinMode(READ, INPUT);

	// IO_SEL is used as interrupt as the waveform for the IO Interaction starts by a falling edge of the IOREQ 
	attachInterrupt(digitalPinToInterrupt(IO_SEL), on_io_interrupt, FALLING);
}

void loop()
{
		set_data_bus_io(0); //basically if not in the interrupt function make sure the databus is set to be a input
}

// runs only on a falling edge of the IOREQ signal
void on_io_interrupt()
{
	unsigned char data;
	unsigned int addr;
	
	unsigned char WRITE_state = digitalRead(WRITE);
	unsigned char READ_state = digitalRead(READ);
	addr = get_addr();
	data = WRITE_state == 0 ? get_data_bus() : data;	
	switch (addr) // Deals with the registers that are allocated to this device
	{
		case IO_TYPE_REG: // for the setting/getting gpio type
			Serial.println("in setting type register");
			if (WRITE_state == 0)
				set_type(data);
			else
				set_data_bus(get_type);
		break;
		case IO_VALS_REG: // for setting/getting gpio output
			Serial.println("setting output register");
			if (WRITE_state == 0)
				set_output(data);
			else
				set_data_bus(get_output);
		break;
	}
}

void set_data_bus_io(unsigned char state) // used to convert the databus to a be input or an output. When the gpio device is inactive or just latching, the databus lines will be set to high impediance (or inputs)
{
	char i;
	for (i=0; i< 8; i++)
		pinMode(data_pins[i], state == 0 ? INPUT: OUTPUT);
}

void set_data_bus(unsigned char data) // used for storing data on the databus
{
	if (digitalRead(WRITE) == 1 && digitalRead(READ) == 0)
	{
		set_data_bus_io(1);
		unsigned char i;
		for (i=0; i< 8; i++)
			digitalWrite(data_pins[i], get_bit(data, i));	
	}
}

unsigned char get_data_bus() // used for getting the data off of the databus
{
	if (digitalRead(WRITE) == 0 && digitalRead(READ) == 1)
	{
		set_data_bus_io(0);
		unsigned char i;
		unsigned char data =0;
		for (i=0; i< 8; i++)
			data |= digitalRead(data_pins[i]) << i;
		return data;
	}
		return 0;
}

unsigned char get_addr() // used for getting the current address
{
	unsigned char i;
	unsigned char addr = 0;
	for (i=0; i< ADDR_PIN_COUNT; i++)
		addr += digitalRead(addr_pins[i]) << i;
	return addr;
}

unsigned char get_bit(unsigned char data, unsigned char index) { return index > 7 ? 0 : ((data & (1 << index)) >> index); } //used to get individual bits from given char

void set_type(unsigned char type) // used to set a given gpio to a desired type
{
	unsigned char i;
	gpio_types = 0;

	for (i=0; i< GPIO_COUNT; i++)
	{
		gpio_types |= get_bit(type, i) << i;
		pinMode(gpio_pins[i], get_bit(gpio_types,i) == 0 ? INPUT : OUTPUT);
	}
}

unsigned char get_type() //returns the gpio type (input or output)
{
	return gpio_types;
}

//gets current state of the pins (no distinction between input or output pins)
unsigned char get_output() 
{
	unsigned char i;
	char gpio_current_state = gpio_current_output;
	for (i=0; i<GPIO_COUNT; i++)
	{
		if (get_bit(gpio_types,i) == 0) // checks to see if a given pin is a input
			gpio_current_state = digitalRead(gpio_pins[i]) == 1 ? (gpio_current_state | (1 << i)) : (gpio_current_state & ~(1 << i));
	}
	return gpio_current_output;
}

void set_output(unsigned char state) // sets the outputs to gpios which have been set to OUTPUT types. The gpio_current_output for inputs are not affected by this.
{
	unsigned char i;

	for (i=0; i< GPIO_COUNT; i++)
	{
		if (get_bit(gpio_types,i) == 1)
	 	{
			gpio_current_output = get_bit(state, i) == 1 ? (gpio_current_output | (1 <<i)) : (gpio_current_output & ~(1 << i));
			digitalWrite(gpio_pins[i],get_bit(gpio_current_output,i));
		}
	}
}
