

/**
 * This example demonstrates pushing a NDEF message from Arduino + NFC Shield to Android 4.0+ device
 *
 * This demo does not support UNO, because UNO board has only one HardwareSerial.
 * Do not try to use SoftwareSerial to control PN532, it won't work. 
 * SotfwareSerial is not fast and stable enough.
 * 
 * This demo only supports the Arduino board which has at least 2 Serial, 
 * Like Leonard(1 USB serial and 1 Hardware serial), Mega ect.
 *
 * Make sure your PN532 board is in HSU(High Speed Uart) mode.
 *
 * This demo is tested with Leonard.
 */

#include <PN532.h>
#include <PN532_HSU.h>
#include <Ndef.h>



/** Use hardware serial to control PN532 */
// PN532                    Arduino
// VCC            -->          5V
// GND            -->         GND
// RXD            -->      Serial1-TX
// TXD            -->      Serail1-RX
/** Serial1 can be  */
#if UNO
PN532_HSU nfc(Serial);
#else
PN532_HSU nfc(Serial1);
#endif

#if !UNO
  #define logPrint(args) Serial.print(args)
  #define logPrintln(args) Serial.println(args)
  #define logPrintHex(n) Serial.print(n, HEX)
#else
  #define logPrint(args)
  #define logPrintln(args)
  #define logPrintHex(n)
#endif

uint8_t pb[256];

void setup(void) {
  
  #if !UNO
  Serial.begin(115200);
  while(!Serial);
  #endif
  logPrintln(F("----------------- NFC RESEARCH --------------------"));

  nfc.begin();
  nfc.wakeup();

  /*
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    //Serial.print("Didn't find PN53x board");
    while (1); // halt
  }
  */
  
  pb[0] = 0x02;
  
  // get firmware version
  logPrintln("get firmware version...");
  nfc.writeCommand((const uint8_t*)&pb, 1, (const uint8_t*)&pb, 0);
  
  nfc.readResponse(pb, 120, 3000);
  
  //PrintHex((const byte*)pb, 6);
  
  #if !UNO
  // Got ok data, print it out!
  Serial.print("Found chip PN5"); 
  Serial.println(pb[0], HEX);
  Serial.print("Firmware ver. "); 
  Serial.print(pb[1], DEC);
  Serial.print('.'); 
  Serial.println(pb[2], DEC);
  Serial.print("Supports "); 
  Serial.println(pb[3], HEX);
  #endif

}



void loop(void) 
{
  /*Serial.println();
  Serial.println(F("---------------- LOOP ----------------------"));
  Serial.println();*/
  
  //test_tag();
  //test_phone();
  
  delay(1000);
  
}




