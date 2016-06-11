/*
 * HM-Sensor-Test2.cpp
 *
 * Created: 27.12.2015 14:47:56
 * Author : Martin
 */

//- load library's --------------------------------------------------------------------------------------------------------
#include <Arduino.h>
#include <AS.h>																				// ask sin framework
#include <THSensor.h>
#include "register.h"																		// configuration sheet
#include "OneWire.h"
#include <avr/wdt.h>

#define SER_DBG

#define OW_PWR		9																		// power for DS18B20
#define OW_PIN		5																		// this pin DS18B20 is connected to

#define SENSOR_COUNT 4

OneWire OW(OW_PIN);
waitTimer thTimer;

void printAddress(byte* address);

typedef struct {
	int16_t celsius;
	byte addr[8];
} Sensor;

Sensor *sensor[SENSOR_COUNT];

void serialEvent();
void lookUpSensors();

//- arduino functions -----------------------------------------------------------------------------------------------------
void setup() {

	for (byte i = 0; i < SENSOR_COUNT; i++) {
		sensor[i] = (Sensor *) malloc(sizeof(Sensor));
	}
	// - Hardware setup ---------------------------------------
	// - everything off ---------------------------------------

	wdt_disable();
	// clear WDRF to avoid endless resets after WDT reset
	MCUSR &= ~(1 << WDRF);							// stop all WDT activities
	WDTCSR |= (1 << WDCE) | (1 << WDE);
	WDTCSR = 0x00;

	EIMSK = 0;									// disable external interrupts
	ADCSRA = 0;														// ADC off
	power_all_disable();								// and everything else

	DDRB = DDRC = DDRD = 0x00;							// everything as input
	PORTB = PORTC = PORTD = 0x00;								// pullup's off

	// todo: timer0 and SPI should enable internally
	power_timer0_enable();
	power_spi_enable();							// enable only needed functions

	// enable only what is really needed

#ifdef SER_DBG																			// some debug
	dbgStart();													// serial setup
	dbg << F("HB-UW-Sen-TH-OW\n");
	dbg << F(LIB_VERSION_STRING);
	_delay_ms(10);									// ...and some information
#endif

	// - AskSin related ---------------------------------------
	hm.init();										// init the asksin framework
	sei();
	// enable interrupts

	// - user related -----------------------------------------
#ifdef SER_DBG
	dbg << F("HMID: ") << _HEX(HMID, 3) << F(", MAID: ") << _HEX(MAID, 3)
			<< F("\n\n");	// some debug
#endif
}



//- user functions --------------------------------------------------------------------------------------------------------
void initTH1() {											// init the sensor

	pinMode(OW_PWR, OUTPUT);
	pinMode(OW_PIN, INPUT_PULLUP);

	digitalWrite(OW_PWR, 1);
	delay(2000);

	lookUpSensors();
	digitalWrite(OW_PWR, 0);

#ifdef SER_DBG
	dbg << "init th1\n";
#endif
}

// this is called when HM wants to send measured values to peers or master
// due to asynchronous measurement we simply can take the values very quick from variables
void measureTH1(THSensor::s_meas *ptr) {


	int16_t t;

#ifdef SER_DBG
	for (byte i = 0; i < SENSOR_COUNT; i++) {
		dbg << "msTH1 OW-t: " << sensor[i]->celsius << ' ' << _TIME << '\n';
	}
#endif
	// take temp value from DS18B20

	t = sensor[0]->celsius / 10;
	((uint8_t *) &(ptr->temp1))[0] = ((t >> 8) & 0x7F);
	((uint8_t *) &(ptr->temp1))[1] = t & 0xFF;

	t = sensor[1]->celsius / 10;
	((uint8_t *) &(ptr->temp2))[0] = ((t >> 8));
	((uint8_t *) &(ptr->temp2))[1] = t & 0xFF;

	t = sensor[2]->celsius / 10;
	((uint8_t *) &(ptr->temp3))[0] = ((t >> 8));
	((uint8_t *) &(ptr->temp3))[1] = t & 0xFF;

	t = sensor[3]->celsius / 10;
	((uint8_t *) &(ptr->temp4))[0] = ((t >> 8));
	((uint8_t *) &(ptr->temp4))[1] = t & 0xFF;

#ifdef SER_DBG
	//dbg << "msTH1 t: " << DHT.temperature << ", h: " << DHT.humidity << ' ' << _TIME << '\n'; _delay_ms(10);
#endif

	// take humidity value from DHT22
	// ptr->hum = DHT.humidity / 10;
	// fetch battery voltage
	t = hm.bt.getVolts();
	((uint8_t *) &(ptr->bat))[0] = t >> 8;
	((uint8_t *) &(ptr->bat))[1] = t & 0xFF;
}


// this is called regularly - real measurement is done here
void measure() {

	enum mState {
		mInit, mWait, mPwrOn, mStartDS
	};
	static mState state = mWait;

	if (!thTimer.done())
		return;

	if (state == mInit) {				// wait some time till next measurement
		thTimer.set(88000);							// measurement every 90 secs
		state = mWait;
	} else if (state == mWait) {			// power on sensor and wait 1 sec
		//thTimer.set(1000);
		digitalWrite(OW_PWR, 1);								// power on here

		state = mPwrOn;
#ifdef SER_DBG
		dbg << "power on Sensor" << ' ' << _TIME << '\n';
#endif
	} else if (state == mPwrOn) {			// now start measurement on DS18B20



		for (byte i = 0; i < SENSOR_COUNT; i++) {
			OW.reset();// attention - OW device get ready to communicate!
			OW.select(sensor[i]->addr);	// skip rom selection - we have only one device attached
			OW.write(0x44,1);
			delay(1000);
			OW.reset();		// attention - get ready to read result from DS18B20
			OW.select(sensor[i]->addr);						// no rom selection;
			OW.write(0xBE);							// read temp from scratchpad
			int16_t celsius = ((uint32_t) (OW.read() | (OW.read() << 8))	* 100) >> 4;
			sensor[i]->celsius = celsius;
			Serial.print(celsius);
			Serial.println(" Celsius:");
		}

		digitalWrite(OW_PWR, 0);							// power off DS18B20
#ifdef SER_DBG
		dbg << "power off Sensor" << ' ' << _TIME << '\n';
		_delay_ms(10);
#endif
		state = mInit;
	}
}

int main(void) {
	// Initialize all functions and pins
	setup();

	/* Replace with your application code */
	while (1) {
		// - AskSin related ---------------------------------------
		hm.poll();								// poll the homematic main loop

		// - user related -----------------------------------------
		serialEvent();
		measure();
	}
}

//- predefined functions --------------------------------------------------------------------------------------------------
void serialEvent() {
#ifdef SER_DBG

	static uint8_t i = 0;						// it is a high byte next time
	while (Serial.available()) {
		uint8_t inChar = (uint8_t) Serial.read();				// read a byte
		if (inChar == '\n') {						// send to receive routine
			i = 0;
			hm.sn.active = 1;
		}

		if ((inChar > 96) && (inChar < 103))
			inChar -= 87;									// a - f
		else if ((inChar > 64) && (inChar < 71))
			inChar -= 55;									// A - F
		else if ((inChar > 47) && (inChar < 58))
			inChar -= 48;									// 0 - 9
		else
			continue;

		if (i % 2 == 0)
			hm.sn.buf[i / 2] = inChar << 4;							// high byte
		else
			hm.sn.buf[i / 2] |= inChar;								// low byte

		i++;
	}
#endif
}

void lookUpSensors() {

	byte address[8];
	byte count = 0;
	byte ok = 0, tmp = 0;

	Serial.println("--Suche gestartet--");
	while (OW.search(address)) {
		tmp = 0;
		//0x10 = DS18S20
		if (address[0] == 0x10) {
			Serial.print("Device is a DS18S20 : ");
			tmp = 1;
		} else {
			//0x28 = DS18B20
			if (address[0] == 0x28) {
				Serial.print("Device is a DS18B20 : ");
				tmp = 1;
			}
		}
		//display the address, if tmp is ok
		if (tmp == 1) {
			if (OneWire::crc8(address, 7) != address[7]) {
				Serial.println("but it doesn't have a valid CRC!");
			} else {
				printAddress(address);
				//all is ok, store it
				memcpy(sensor[count]->addr, address, 8);
				count++;
				ok = 1;
			}
		}								//end if tmp
	}								//end while
	if (ok == 0) {
		Serial.println("Keine Sensoren gefunden");
	}
	Serial.println("--Suche beendet--");

}

void printAddress(byte* address) {
	for (byte i = 0; i < 8; i++) {
		if (address[i] < 9) {
			Serial.print("0");
		}
		Serial.print("0x");
		Serial.print(address[i], HEX);
		if (i < 7) {
			Serial.print(", ");
		}
	}
	Serial.println("");
}

