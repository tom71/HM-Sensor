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

OneWire OW(OW_PIN);
waitTimer thTimer;


byte i;
byte sensorCount=3;


typedef struct {
	int16_t celsius;
	byte addr[8];
} Sensor;

Sensor *sensor[4];


void serialEvent();

//- arduino functions -----------------------------------------------------------------------------------------------------
void setup() {
	for(i=0; i<sensorCount;i++){
		sensor[i] = (Sensor *)malloc(sizeof(Sensor));
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

#ifdef SER_DBG
	dbg << "init th1\n";
#endif
}

// this is called when HM wants to send measured values to peers or master
// due to asynchronous measurement we simply can take the values very quick from variables
void measureTH1(THSensor::s_meas *ptr) {

	byte i;
	int16_t t;

	#ifdef SER_DBG
		for(i=0; i<4; i++)
		{
			dbg << "msTH1 OW-t: " << sensor[i]->celsius << ' ' << _TIME << '\n';
		}
	#endif
	// take temp value from DS18B20

	t = sensor[0]->celsius / 10;
	((uint8_t *) &(ptr->temp1))[0] = ((t >> 8) & 0x7F);
	((uint8_t *) &(ptr->temp1))[1] = t & 0xFF;

	t = sensor[1]->celsius / 10;
	((uint8_t *) &(ptr->temp2))[0] = ((t >> 8) & 0x7F);
	((uint8_t *) &(ptr->temp2))[1] = t & 0xFF;

	t = sensor[2]->celsius / 10;
	((uint8_t *) &(ptr->temp3))[0] = ((t >> 8) & 0x7F);
	((uint8_t *) &(ptr->temp3))[1] = t & 0xFF;

	/*
	t = sensor[3]->celsius / 10;
	((uint8_t *) &(ptr->temp4))[0] = ((t >> 8) & 0x7F);
	((uint8_t *) &(ptr->temp4))[1] = t & 0xFF;
	*/

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
		thTimer.set(1000);
		digitalWrite(OW_PWR, 1);								// power on here

		for (i = 0; i < 4; i++) {
			OW.search(sensor[i]->addr);
			#ifdef SER_DBG

				byte j;
				Serial.println("Sensor address=");
				for (j = 0; j < 8; j++) {
					Serial.print(sensor[i]->addr[j], HEX);
					Serial.print(" ");
				}
				Serial.println("");
			#endif
			delay(250);
		}

		state = mPwrOn;
		#ifdef SER_DBG
				dbg << "power on Sensor" << ' ' << _TIME << '\n';
		#endif
	} else if (state == mPwrOn) {// now start measurement on DS18B20
		byte present = 0;

		for (i = 0; i < 4; i++) {
			OW.reset(); // attention - get ready to read result from DS18B20
			OW.select(sensor[i]->addr);

			OW.write(0x44, 1); // start conversion, with parasite power on at the end

			delay(1000);     // maybe 750ms is enough, maybe not
			// we might do a ds.depower() here, but the reset will take care of it.

			present = OW.reset();
			OW.select(sensor[i]->addr);
			OW.write(0xBE);         // Read Scratchpad

			Serial.print("P=");
			Serial.println(present, HEX);

			byte data[12];

			sensor[i]->celsius = ((uint32_t) (OW.read() | (OW.read() << 8))
					* 100) >> 4; // we need only first two bytes from scratchpad
			#ifdef SER_DBG
				dbg << "ms OW-t: " << sensor[i]->celsius << ' ' << _TIME << '\n';
			#endif
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
