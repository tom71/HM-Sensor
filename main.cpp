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
#include "dht.h"
#include "OneWire.h"

//#define SER_DBG

#define DHT_PWR		9																		// power for DHT22
#define DHT_PIN		6																		// this pin DHT22 is connected to
#define OW_PIN		5																		// this pin DS18B20 is connected to

dht DHT;
OneWire OW(OW_PIN);
waitTimer thTimer;
int16_t celsius;

void serialEvent();


//- arduino functions -----------------------------------------------------------------------------------------------------
void setup() {

	// - Hardware setup ---------------------------------------
	// - everything off ---------------------------------------

	EIMSK = 0;																				// disable external interrupts
	ADCSRA = 0;																				// ADC off
	power_all_disable();																	// and everything else
	
	DDRB = DDRC = DDRD = 0x00;																// everything as input
	PORTB = PORTC = PORTD = 0x00;															// pullup's off

	// todo: timer0 and SPI should enable internally
	power_timer0_enable();
	power_spi_enable();																		// enable only needed functions

	// enable only what is really needed

	#ifdef SER_DBG																			// some debug
		dbgStart();																			// serial setup
		dbg << F("HM_LC_SW1_BA_PCB\n");
		dbg << F(LIB_VERSION_STRING);
		_delay_ms (10);																		// ...and some information
	#endif
	
	// - AskSin related ---------------------------------------
	hm.init();																				// init the asksin framework
	sei();																					// enable interrupts

	// - user related -----------------------------------------
	#ifdef SER_DBG
		dbg << F("HMID: ") << _HEX(HMID,3) << F(", MAID: ") << _HEX(MAID,3) << F("\n\n");	// some debug
	#endif
}


//- user functions --------------------------------------------------------------------------------------------------------
void initTH1() {																			// init the sensor
	
	pinMode(DHT_PWR, OUTPUT);
	pinMode(OW_PIN, INPUT_PULLUP);
	
	#ifdef SER_DBG
		dbg << "init th1\n";
	#endif
}

void measureTH1(THSensor::s_meas *ptr) {
	int16_t t;
	
	#ifdef SER_DBG
		dbg << "msTH1 DS-t: " << celsius << ' ' << _TIME << '\n';
	#endif
	t = celsius / 10;
	((uint8_t *)&(ptr->temp))[0] = ((t >> 8) & 0x7F) | (hm.bt.getStatus() << 7);
	((uint8_t *)&(ptr->temp))[1] = t & 0xFF;
	
	#ifdef SER_DBG
		dbg << "msTH1 t: " << DHT.temperature << ", h: " << DHT.humidity << ' ' << _TIME << '\n'; _delay_ms(10);
	#endif
	ptr->hum = DHT.humidity / 10;
	t = hm.bt.getVolts();
	((uint8_t *)&(ptr->bat))[0] = t >> 8;
	((uint8_t *)&(ptr->bat))[1] = t & 0xFF;
}

void measure() {
	enum mState {mInit, mWait, mPwrOn, mStartDS};
	static mState state = mWait;

	if (!thTimer.done())
		return;
		
	if (state == mInit) {																	// wait some time till next measurement
		thTimer.set(28000);
		state = mWait;
	}
	else if (state == mWait) {																// power on sensor and wait 1 sec
		thTimer.set(1000);
		digitalWrite(DHT_PWR, 1);
		state = mPwrOn;
		#ifdef SER_DBG
			//dbg << "power on Sensor" << ' ' << _TIME << '\n';
		#endif
	}
	else if (state == mPwrOn) {																// now start measurement on DS18B20 and wait another second
		thTimer.set(1000);
		uint8_t rc = OW.reset();
		OW.skip();
		OW.write(0x44);																		// start conversion
		#ifdef SER_DBG
			//dbg << "rc: " << rc << _TIME << '\n';
		#endif
		state = mStartDS;
	}
	else if (state == mStartDS)	{
		DHT.read22(DHT_PIN);																// read DHT22
		#ifdef SER_DBG
			dbg << "t: " << DHT.temperature << ", h: " << DHT.humidity << ' ' << _TIME << '\n';
		#endif
		
		OW.reset();																			// and read result from DS18B20
		OW.skip();
		OW.write(0xBE);																		// read scratchpad
		celsius = ((uint32_t)(OW.read() | (OW.read() << 8)) * 100) >> 4;					// we need only first two bytes from scratchpad
	
		#ifdef SER_DBG
			dbg << "DS-t: " << celsius << ' ' << _TIME << '\n';
		#endif

		digitalWrite(DHT_PWR, 0);															// power off DHT22
		#ifdef SER_DBG
			//dbg << "power off Sensor" << ' ' << _TIME << '\n';
			_delay_ms(10);
		#endif
		state = mInit;
	}
}


int main(void)
{
	// Initialize all functions and pins
	setup();
	
    /* Replace with your application code */
    while (1) 
    {
			// - AskSin related ---------------------------------------
			hm.poll();																				// poll the homematic main loop

			// - user related -----------------------------------------
			serialEvent();
			measure();
    }
}


//- predefined functions --------------------------------------------------------------------------------------------------
void serialEvent() {
	#ifdef SER_DBG
	
	static uint8_t i = 0;																	// it is a high byte next time
	while (Serial.available()) {
		uint8_t inChar = (uint8_t)Serial.read();											// read a byte
		if (inChar == '\n') {																// send to receive routine
			i = 0;
			hm.sn.active = 1;
		}
		
		if      ((inChar>96) && (inChar<103)) inChar-=87;									// a - f
		else if ((inChar>64) && (inChar<71))  inChar-=55;									// A - F
		else if ((inChar>47) && (inChar<58))  inChar-=48;									// 0 - 9
		else continue;
		
		if (i % 2 == 0) hm.sn.buf[i/2] = inChar << 4;										// high byte
		else hm.sn.buf[i/2] |= inChar;														// low byte
		
		i++;
	}
	#endif
}
