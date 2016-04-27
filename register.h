#ifndef _REGISTER_h
	#define _REGISTER_h

	//- load libraries -------------------------------------------------------------------------------------------------------
	#include <AS.h>                                                         // the asksin framework
	#include "hardware.h"                                                   // hardware definition
	#include <THSensor.h>
	#include "hmkey.h"

	//- stage modules --------------------------------------------------------------------------------------------------------
	AS hm;                                                                  // asksin framework
	THSensor thsens;                                                        // create instance of channel module

	// some forward declarations
	extern void initTH1();
	extern void measureTH1(THSensor::s_meas *);

	/*
	 * HMID, Serial number, HM-Default-Key, Key-Index
	 */
	const uint8_t HMSerialData[] PROGMEM = {
		/* HMID */            0x58, 0x23, 0xFF,
		/* Serial number */   'X', 'M', 'S', '1', '2', '3', '4', '5', '6', '7',
		/* Default-Key */     HM_DEVICE_AES_KEY,
		/* Key-Index */       HM_DEVICE_AES_KEY_INDEX,
	};

	/*
	 * Settings of HM device
	 * firmwareVersion: The firmware version reported by the device
	 *                  Sometimes this value is important for select the related device-XML-File
	 *
	 * modelID:         Important for identification of the device.
	 *                  @See Device-XML-File /device/supported_types/type/parameter/const_value
	 *
	 * subType:         Identifier if device is a switch or a blind or a remote
	 * DevInfo:         Sometimes HM-Config-Files are referring on byte 23 for the amount of channels.
	 *                  Other bytes not known.
	 *                  23:0 0.4, means first four bit of byte 23 reflecting the amount of channels.
	 */
	const uint8_t devIdnt[] PROGMEM = {
		/* firmwareVersion 1 byte */  0x10,
		/* modelID         2 byte */  0xF2,0x01,
		/* subTypeID       1 byte */  0x70,
		/* deviceInfo      3 byte */  0x01, 0x01, 0x00,
	};

	/*
	 * Register definitions
	 * The values are adresses in relation to the start adress defines in cnlTbl
	 * Register values can found in related Device-XML-File.

	 * Spechial register list 0: 0x0A, 0x0B, 0x0C
	 * Spechial register list 1: 0x08
	 *
	 * @See Defines.h
	 *
	 * @See: cnlTbl
	 */
	const uint8_t cnlAddr[] PROGMEM = {
		// List0-Register
		0x01,0x02,0x0a,0x0b,0x0c,
		// List3-Register
		0x01,
	};  // 6 byte

	/*
	* Channel - List translation table
	* channel, list, startIndex, start address in EEprom, hidden
	*/
	EE::s_cnlTbl cnlTbl[] = {
		// cnl, lst, sIdx,  sLen, pAddr,  hidden
		{ 0, 0, 0x00,  5, 0x001f, 0, },
		{ 1, 4, 0x05,  1, 0x0024, 0, },		// 1 reg * 6 peers = 6 byte
	};  // 14 byte

	/*
	* Peer-Device-List-Table
	* channel, maximum allowed peers, start address in EEprom
	*/
	EE::s_peerTbl peerTbl[] = {
		// cnl, peerMax, pAddr;
		{ 1, 6, 0x002a, },					// 6 * 4 = 24 byte
	};	// 4 Byte

	/*
	* handover to AskSin lib
	*
	* TODO: Describe
	*/
	EE::s_devDef devDef = {
		1, 2, devIdnt, cnlAddr,
	};	// 6 Byte

	/*
	* module registrar
	*
	* TODO: Describe
	*/
	RG::s_modTable modTbl[1];


	/**
	* @brief First time and regular start functions
	*/
	void everyTimeStart(void) {
		/*
		* Place here everything which should be done on each start or reset of the device.
		* Typical use case are loading default values or user class configurations.
		*/

		// init the homematic framework
		hm.confButton.config(1, CONFIG_KEY_PCIE, CONFIG_KEY_INT);           // configure the config button, mode, pci byte and pci bit
		hm.ld.init(2, &hm);                                                 // set the led
		hm.ld.set(welcome);                                                 // show something
		//hm.bt.set(30, 3600000);                                             // set battery check, internal, 2.7 reference, measurement each hour
		hm.bt.set(220, 900000);                                             // set battery check, internal, 2.7 reference, measurement each 1/4 hour
		//hm.pw.setMode(POWER_MODE_NO_SLEEP);                                 // set power management mode
		hm.pw.setMode(POWER_MODE_WAKEUP_ONRADIO);                           // set power management mode

	    thsens.regInHM(1, 4, &hm);											// register sensor module on channel 1, with a list4 and introduce asksin instance
	    thsens.config(&initTH1, &measureTH1);								// configure the user class and handover addresses to respective functions and variables
	    thsens.timing(0, 0, 0);												// mode 0 transmit based on timing or 1 on level change; level change value; while in mode 1 timing value will stay as minimum delay on level change
	}

	/**
	* TODO: maybe we can delete this?
	*/
	void firstTimeStart(void) {
	}
#endif
