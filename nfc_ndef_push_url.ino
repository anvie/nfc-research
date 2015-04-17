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

#include <PN5321.h>
#include <NFCLinkLayer.h>
#include <SNEP.h>

#include <NdefMessage.h>

#define UNO 0

#define TS_GET_DATA_IN_MAX_SIZE  262 + 3

/** Use hardware serial to control PN532 */
// PN532                    Arduino
// VCC            -->          5V
// GND            -->         GND
// RXD            -->      Serial1-TX
// TXD            -->      Serail1-RX
/** Serial1 can be  */
#if UNO
PN532 nfc(Serial);
#else
PN532 nfc(Serial1);
#endif
NFCLinkLayer linkLayer(&nfc);
SNEP snep(&linkLayer);


// NDEF messages
#define MAX_PKT_HEADER_SIZE  50
#define MAX_PKT_PAYLOAD_SIZE 100
uint8_t txNDEFMessage[MAX_PKT_HEADER_SIZE + MAX_PKT_PAYLOAD_SIZE];
uint8_t *txNDEFMessagePtr; 
uint8_t txLen;

#if UNO
  #define logPrint(args) Serial.print(args)
  #define logPrintln(args) Serial.println(args)
  #define logPrintHex(n) Serial.print(n, HEX)
#else
  #define logPrint(args)
  #define logPrintln(args)
  #define logPrintHex(n)
#endif

void setup(void) {
  
  #if !UNO
  Serial.begin(115200);
  while(!Serial);
  #endif
  logPrintln(F("----------------- nfc ndef push url --------------------"));


  txNDEFMessagePtr = &txNDEFMessage[MAX_PKT_HEADER_SIZE];
  NdefMessage message = NdefMessage();
  message.addUriRecord("http://www.ansvia.com");
  //message.addTextRecord("XIPP Tx. Rp. 55.000,-");
  txLen = message.getEncodedSize();
  if (txLen <= MAX_PKT_PAYLOAD_SIZE) {
    message.encode(txNDEFMessagePtr);
  } 
  else {
    logPrintln("Tx Buffer is too small.");
    while (1) {
    }
  }

  nfc.initializeReader();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    //Serial.print("Didn't find PN53x board");
    while (1); // halt
  }
  
  #if !UNO
  // Got ok data, print it out!
  Serial.print("Found chip PN5"); 
  Serial.println((versiondata>>24) & 0xFF, HEX);
  Serial.print("Firmware ver. "); 
  Serial.print((versiondata>>16) & 0xFF, DEC);
  Serial.print('.'); 
  Serial.println((versiondata>>8) & 0xFF, DEC);
  Serial.print("Supports "); 
  Serial.println(versiondata & 0xFF, HEX);
  #endif

  nfc.SAMConfig();
}


uint8_t packetBuffer[TS_GET_DATA_IN_MAX_SIZE];

void test_phone(){
  uint32_t txResult = GEN_ERROR;

  if (IS_ERROR(nfc.configurePeerAsTarget(SNEP_SERVER))) {
    extern uint8_t pn532_packetbuffer[];

    logPrintln(F("\nSNEP Sever:Blocking wait response."));
    nfc.readspicommand(PN532_TGINITASTARGET, (PN532_CMD_RESPONSE *)pn532_packetbuffer, 3000);
  }
  /*
  packetBuffer[0] = PN532_INJUMPFORDEP;
  packetBuffer[1] = 0x01;
  packetBuffer[2] = 0x02;
  packetBuffer[3] = 0x00;
  nfc.sendCommandCheckAck((uint8_t*)&packetBuffer, 5, 1000);
  */

  logPrintln("pushPayload");

  txResult = snep.pushPayload(txNDEFMessagePtr, txLen);
  logPrint(F("Result: 0x"));
  logPrintHex(txResult);
  logPrint(" ");  
  logPrint(NFCReader::errorString(txResult));
  if(txResult == 0x00000001){
    delay(1000);
  }
}
  

void test_tag(){
  uint32_t cid = nfc.readPassiveTargetID(0x00);
  logPrint("test_tag got cid: ");
  logPrintln(cid);
  
  /*
  packetBuffer[0] = PN532_INLISTPASSIVETARGET;
  packetBuffer[1] = 1;
  packetBuffer[2] = 0x00;
  
  if(IS_ERROR(nfc.sendCommandCheckAck(packetBuffer, 3))){
    logPrintln("read tag error!");
  }
  
  PN532_CMD_RESPONSE *response = (PN532_CMD_RESPONSE *) pn532_packetbuffer;
  if (IS_ERROR(readspicommand(PN532_INDATAEXCHANGE, response)))
  {
     return 0;
  }
  */

}


void loop(void) 
{
  /*Serial.println();
  Serial.println(F("---------------- LOOP ----------------------"));
  Serial.println();*/
  
  //test_tag();
  test_phone();
  
  delay(1000);
  
}




