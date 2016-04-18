//- ----------------------------------------------------------------------------------------------------------------------
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

//- ----------------------------------------------------------------------------------------------------------------------
//- eeprom defaults table ------------------------------------------------------------------------------------------------
uint16_t EEMEM eMagicByte;
uint8_t  EEMEM eHMID[3]  = {0x58,0x23,0xff,};
uint8_t  EEMEM eHMSR[10] = {'X','M','S','1','2','3','4','5','6','7',};
uint8_t  EEMEM eHMKEY[16] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,};

/*
	* HMID, Serial number, HM-Default-Key, Key-Index
	*/
	const uint8_t HMSerialData[] PROGMEM = {
		/* HMID */            0x58, 0x23, 0xFF,
		/* Serial number */   'X', 'M', 'S', '1', '2', '3', '4', '5', '6', '7',
		/* Default-Key */     HM_DEVICE_AES_KEY,
		/* Key-Index */       HM_DEVICE_AES_KEY_INDEX,
	};
	
// if HMID and Serial are not set, then eeprom ones will be used
//uint8_t HMID[3] = {0x58,0x23,0xff,};
//uint8_t HMSR[10] = {'X','M','S','1','2','3','4','5','6','7',};          // XMS1234567
//uint8_t HMKEY[16] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,};

//- ----------------------------------------------------------------------------------------------------------------------
//- settings of HM device for AS class -----------------------------------------------------------------------------------
const uint8_t devIdnt[] PROGMEM = {
	/* Firmware version  1 byte */  0x10,                               // don't know for what it is good for
	/* Model ID          2 byte */  0xF2,0x01,                          // model ID, describes HM hardware. Own devices should use high values due to HM starts from 0
	/* Sub Type ID       1 byte */  0x70,                               // not needed for FHEM, it's something like a group ID
	/* Device Info       3 byte */  0x01,0x01,0x00,                     // describes device, not completely clear yet. includes amount of channels
};  // 7 byte

//- ----------------------------------------------------------------------------------------------------------------------
//- channel slice address definition -------------------------------------------------------------------------------------
const uint8_t cnlAddr[] PROGMEM = {
	0x01,0x02,0x0a,0x0b,0x0c,
	0x01,
};  // 6 byte

//- channel device list table --------------------------------------------------------------------------------------------
EE::s_cnlTbl cnlTbl[] = {
	// cnl, lst, sIdx, sLen, pAddr, hidden
	{ 0, 0, 0x00,  5, 0x000f, 0, },
	{ 1, 4, 0x05,  1, 0x0014, 0, },		// 1 reg * 6 peers = 6 byte
};  // 14 byte

//- peer device list table -----------------------------------------------------------------------------------------------
EE::s_peerTbl peerTbl[] = {
	// cnl, pMax, pAddr;
	{ 1, 6, 0x001a, },					// 6 * 4 = 24 byte
};  // 4 byte

//- handover to AskSin lib -----------------------------------------------------------------------------------------------
EE::s_devDef devDef = {
	1, 2, devIdnt, cnlAddr,
};  // 6 byte

//- module registrar -----------------------------------------------------------------------------------------------------
RG::s_modTable modTbl[1];

//- ----------------------------------------------------------------------------------------------------------------------
//- first time and regular start functions -------------------------------------------------------------------------------

void everyTimeStart(void) {
	// place here everything which should be done on each start or reset of the device
	// typical use case are loading default values or user class configurations

	// init the homematic framework
	hm.confButton.config(1, CONFIG_KEY_PCIE, CONFIG_KEY_INT);           // configure the config button, mode, pci byte and pci bit
	hm.ld.init(2, &hm);                                                 // set the led
	hm.ld.set(welcome);                                                 // show something
//	hm.bt.set(30, 3600000);                                             // set battery check, internal, 2.7 reference, measurement each hour
	hm.bt.set(200, 120000);                                              // set battery check, internal, 2.7 reference, measurement each hour
	hm.pw.setMode(1);                                                   // set power management mode

    // register user modules
    //cmSwitch[0].regInHM(1, 3, &hm);                                    // register user module
    //cmSwitch[0].config(&initRly, &switchRly);                          // configure user module
	thsens.regInHM(1, 4, &hm);											// register sensor module on channel 1, with a list4 and introduce asksin instance
	thsens.config(&initTH1, &measureTH1/*, &thVal*/);						// configure the user class and handover addresses to respective functions and variables
	thsens.timing(0, 0, 0);												// mode 0 transmit based on timing or 1 on level change; level change value; while in mode 1 timing value will stay as minimum delay on level change
}

void firstTimeStart(void) {
	// place here everything which should be done on the first start or after a complete reset of the sketch
	// typical use case are default values which should be written into the register or peer database

}
