

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
#include <NfcAdapter.h>
#include <Ndef.h>

#define IS_DEBUG           1
#define VERSION_CHECKER    0
#define TEST_TAG           0
#define TEST_SNEP          1

#include <debug.h>


#if TEST_SNEP
#include <snep.h>
#endif


/** Use hardware serial to control PN532 */
// PN532                    Arduino
// VCC            -->          5V
// GND            -->         GND
// RXD            -->      Serial1-TX
// TXD            -->      Serail1-RX
/** Serial1 can be  */
#if UNO
PN532_HSU pn532(Serial);
#else
PN532_HSU pn532(Serial1);
#endif

#if !TEST_SNEP
NfcAdapter nfc(pn532);
#else
SNEP nfc(pn532);
#endif

#if VERSION_CHECKER
uint8_t pb[256];
#endif

void checkVersion();
void scanTag();
#if TEST_SNEP
void doSNEP();
#endif

void setup(void) {
  
  #if !UNO
  Serial.begin(115200);
  #if IS_DEBUG
  while(!Serial);
  #endif // IS_DEBUG
  #endif // !UNO
  MSGPRINT(F("----------------- NFC RESEARCH --------------------\n"));
  
  #if TEST_SNEP
  MSGPRINT(F("P2P TESTING\n"));
  #endif
  
  #if !TEST_SNEP
  pn532.begin();
  pn532.wakeup();

  nfc.begin();
  #endif
  
  #if VERSION_CHECKER
  checkVersion();
  #endif

}



void loop(void) 
{
  #if TEST_TAG
  scanTag();
  #endif
  
  #if TEST_SNEP
  doSNEP();
  #endif
  
  delay(1000);
  
}

#if TEST_SNEP
uint8_t ndefBuf[128];
bool retry = false;

uint8_t paySign[2] = {'P',':'};

bool isToPay(){
  uint32_t i = 0;
  while (Serial.available() > 0){
    if (Serial.read() != paySign[i++])
      return false;
    else if (i == sizeof(paySign))
      return true;
  }
  return false;
} 

void doSNEP(){
#if 1

  if (isToPay()){
    retry = true;
    
    char buf[64];
    memset(&buf, 0, 64);
    
    for (uint8_t i = 0; Serial.available() > 0; i++){
      buf[i] = Serial.read();
    }
    
    uint32_t retried = 0;
    
    while (retry && retried < 7){
      DMSG(retried + 1);DMSG(". ");
      DMSG(F("Send a message to Android: "));
      DMSG(buf);

      NdefMessage message = NdefMessage();
    
      char tmp[100];
      //sprintf(tmp, "http://www.ansvia.com?r=%d", abs(random(1000)));
      sprintf(tmp, "http://www.ansvia.com?r=%s", buf);
    
      message.addUriRecord(String(tmp));
      
      int messageSize = message.getEncodedSize();
      if (messageSize > sizeof(ndefBuf)) {
          MSGERROR(F("ndefBuf is too small\n"));
          while (1) {
          }
  
      }
  
      message.encode(ndefBuf);
      if (0 >= nfc.write(ndefBuf, messageSize, 5000)) {
        retry = true;
        retried++;
        MSGERROR(F(" Failed\n"));
      } else {
        retry = false;
        MSGPRINT(F(" Success\n"));
        delay(3000);
      }
    }
    if (retried >= 7)
      DMSG(F("timeout."));
    
    
  }

#else
    DMSG(F("Get a message from Android"));
    int msgSize = nfc.read(ndefBuf, sizeof(ndefBuf), 3000);
    if (msgSize > 0) {
        NdefMessage msg  = NdefMessage(ndefBuf, msgSize);
        msg.print();
        DMSG("\nSuccess");
        delay(3000);
    } else {
        MSGPRINT(F("failed\n"));
    }
#endif
}
#endif

#if TEST_TAG
void scanTag(){
  //DMSG("scanning tag...\n");
  if (nfc.tagPresent()){
    NfcTag tag = nfc.read();
    tag.print();
    
    DMSG("trying to write...\n");
    NdefMessage ndef = NdefMessage();
    
    char* tmp[100];
    sprintf(tmp, "http://www.ansvia.com?r=%d", random(1, 100000));
    
    ndef.addUriRecord(tmp);
    if (nfc.write(ndef)){
      DMSG("success writing to tag\n");
    }else{
      MSGERROR("write failed");
    }
  }
}
#endif

#if VERSION_CHECKER
void checkVersion(){
  pb[0] = 0x02;
  
  // get firmware version
  MSGPRINT(F("get firmware version...\n"));
  pn532.writeCommand((const uint8_t*)&pb, 1, (const uint8_t*)&pb, 0);
  
  pn532.readResponse(pb, 120, 3000);
  
  //PrintHex((const byte*)pb, 6);
  
  #if !UNO
  // Got ok data, print it out!
  MSGPRINT(F("  Found chip PN5")); 
  MSGPRINT_HEX(pb[0]);
  MSGPRINT("\n");
  MSGPRINT(F("  Firmware ver. ")); 
  MSGPRINT_DEC(pb[1]);
  MSGPRINT('.'); 
  MSGPRINT_DEC(pb[2]);NL;
  MSGPRINT(F("  Supports ")); 
  MSGPRINT_HEX(pb[3]);NL;
  #endif
}
#endif


