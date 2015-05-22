

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
#include <PN532Interface.h>
#include <Ndef.h>

#define UNO                0
#define IS_DEBUG           1
#define VERSION_CHECKER    0
#define TEST_TAG           0
#define TEST_WRITE_TAG     0 // need TEST_TAG
#define TEST_SNEP          0
#define TEST_HCE           1
#define USING_LCD          1
#define LCD_8X2            0
#define LCD_16X2           1
#define USING_LED          1


#include <debug.h>


#if TEST_SNEP
#include <snep.h>
#endif

#if USING_LCD
#include <LiquidCrystal.h>

LiquidCrystal lcd(15, 14, 16, 4, 5, 6, 7);
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

#if TEST_SNEP
    SNEP nfc(pn532);
#elif TEST_HCE
    PN532 nfc(pn532);
#else
    NfcAdapter nfc(pn532);
#endif

#if VERSION_CHECKER
uint8_t pb[256];
#endif

void checkVersion();
void scanTag();
#if TEST_SNEP
void doSNEP();
#endif
#if USING_LED
void dim_led(uint16_t led_num, uint16_t count);
#endif

void setup(void) {
  
  #if !UNO
  Serial.begin(115200);
  #if IS_DEBUG
  while(!Serial);
  #endif // IS_DEBUG
  #endif // !UNO
  
  #if USING_LED
  #if UNO
  pinMode(12, OUTPUT);
  dim_led(12, 1);
  #endif
  // digunakan untuk indikasi kalo ada mobile device approach
  pinMode(9, OUTPUT);
  #endif
  
  
  MSGPRINT(F("----------------- NFC RESEARCH --------------------\n"));
  
  #if TEST_SNEP
  MSGPRINT(F("P2P TESTING\n"));
  #endif
  
  #if !TEST_SNEP
  pn532.begin();
  pn532.wakeup();

  nfc.begin();
  #endif
  
  #if USING_LED
  #if UNO
  dim_led(12, 2);
  #endif
  #endif
  
  
  #if VERSION_CHECKER
  checkVersion();
  #endif
  
  #if TEST_HCE
    // configure board to read RFID tags
    nfc.SAMConfig();
  #endif
  
  #if USING_LCD
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  
  digitalWrite(2, HIGH);
  digitalWrite(3, HIGH);
  
  #if LCD_8X2
  lcd.begin(8, 2);
  #elif LCD_16X2
  lcd.begin(16, 2);
  #endif
  
  lcd.setCursor(0,0);
  lcd.print("~  XIPP READY  ~");
  #endif


}

uint8_t ping_sign[4] = {'X','I','P','P'};

bool is_ping_sign(){
  uint32_t i = 0;
  while (Serial.available() > 0){
    if (Serial.read() != ping_sign[i++])
      return false;
    else if (i == sizeof(ping_sign))
      return true;
  }
  return false;
}

void loop(void) 
{
  
  if (is_ping_sign()){
    Serial.println("XIPP\n");
  }
  
  
  #if TEST_TAG
  scanTag();
  #endif
  
  #if TEST_SNEP
  doSNEP();
  #endif
  
  #if TEST_HCE
  doHCE();
  #endif
  
  delay(1000);
  
}

#if TEST_SNEP || TEST_HCE
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

#if USING_LCD
void clearLcdLine(int num){
  lcd.setCursor(0,num);
  lcd.print(F("                "));
}
#endif
#endif // TEST_SNEP || TEST_HCE

#if TEST_SNEP
void doSNEP(){
#if 1

  if (isToPay()){
    
    retry = true;
    
    char buf[64];
    memset(&buf, 0, 64);
    
    for (uint8_t i = 0; Serial.available() > 0; i++){
      buf[i] = Serial.read();
    }
    
    #if USING_LCD
    lcd.setCursor(0,0);
    lcd.print(F("Total (IDR):    "));
    clearLcdLine(1);
    lcd.setCursor(0,1);
    lcd.print("Rp. ");
    lcd.setCursor(4,1);
    lcd.print(buf);
    #endif
    
    uint32_t retried = 0;
    
    while (retry && retried < 10){
      DMSG(retried + 1);DMSG(". ");
      DMSG(F("Send a message to Android: "));
      DMSG(buf);
      
      #if USING_LCD
      lcd.setCursor(15,1);
      if (retried % 2 == 0){
        lcd.print("*");
      }else{
        lcd.print(" ");
      }
      #endif

      NdefMessage message = NdefMessage();
    
      char tmp[64];
      sprintf(tmp, "http://xipp.info?r=%s", buf);
    
      message.addUriRecord(String(tmp));
      
      int messageSize = message.getEncodedSize();
      if (messageSize > sizeof(ndefBuf)) {
          MSGERROR(F("ndefBuf is too small\n"));
          while (1) {
          }
  
      }
  
      message.encode(ndefBuf);
      int8_t rv = nfc.write(ndefBuf, messageSize, 5000);
      if (0 >= rv) {
        retry = true;
        retried++;
        MSGERROR(F(" Failed, ret: "));
        MSGPRINT(rv);NL;
        
        
        if (rv < -1){
          #if USING_LCD
          lcd.setCursor(0,1);
          lcd.print("...PROCESSING...");
          #endif
          #if USING_LED
          dim_led(9, 5);
          #endif
        }
        
        
        
        delay(700);
      } else {
        retry = false;
        Serial.println(F("SUCCESS"));
        
        // temporary send dummy ack
        //
        delay(2000);
        
        // dapatkan data dari opposite PN532
        memset(&ndefBuf, 0, 128);
        
        int msgSize = nfc.read(ndefBuf, 128, 60000);
        if (msgSize > 0) {
            NdefMessage msg = NdefMessage(ndefBuf, msgSize);
            //msg.print();

            byte payload[256];
            memset(&payload, 0, 250);
            
            msg.getRecord(0).getPayload((byte*)&payload);
            
            Serial.println("DONE|" + String((const char*)&payload));
            

            //Serial.print((char*)&payload);
            //Serial.println();
            
            #if USING_LCD
            clearLcdLine(1);
            lcd.setCursor(0,0);
            lcd.print(F("PAYMENT SUCCESS "));
            lcd.setCursor(0,1);
            lcd.print(F("   THANK YOU    "));
            #endif
            
            DMSG("\nSuccess");
            delay(3000);
        } else {
            MSGERROR("nfc read return: ");
            MSGPRINT(msgSize);
            MSGPRINT(F("Failed code 405\n"));
        }

        
        
        delay(10000);
        #if USING_LCD
        lcd.setCursor(0,1);
        clearLcdLine(1);
        lcd.setCursor(0,0);
        lcd.print("~ XIPP READY   ");
        clearLcdLine(1);
        #endif
      }
    }
    if (retried >= 7){
      DMSG(F("timeout."));
      #if USING_LCD
      lcd.setCursor(0,0);
      lcd.print("~ XIPP READY   ");
      clearLcdLine(1);
      lcd.setCursor(0,1);
      lcd.print("timeout.");
      #endif
    }
    
    
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

#if TEST_HCE
void doHCE(){

  if (isToPay()) {
    bool success;

    uint8_t response[74];
    uint8_t responseLength = 32;

    //get payment data from serial
    char buf[64];

    memset(&buf, 0, 64);

    size_t n = 0;

    for (uint8_t i = 0; Serial.available() > 0; i++){
      buf[i] = Serial.read();
      n++;
    }
    //DMSG(F("ukurannya\n"));
    //DMSG(n);
    //uint8_t buf_i[n];
    
    //memcpy(buf_i, buf, n);
    
    //display to LCD
    #if USING_LCD
    lcd.setCursor(0,0);
    lcd.print(F("Total (IDR):    "));
    clearLcdLine(1);
    lcd.setCursor(0,1);
    lcd.print("Rp. ");
    lcd.setCursor(4,1);
    lcd.print(buf);
    #endif
    
    DMSG(F("Waiting for an Android tap\n"));
    
    // set shield to inListPassiveTarget
    success = nfc.inListPassiveTarget();
  
    if(success) {
     
       DMSG(F("Found something!\n"));
                    
      uint8_t selectApdu[] = { 0x00, /* CLA */
                                0xA4, /* INS */
                                0x04, /* P1  */
                                0x00, /* P2  */
                                0x07, /* Length of AID  */
                                0xF0, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06//, /* AID defined on Android App */
                                /*0xF0 // Le  */};
      uint8_t getDataApdu[] = {0x00, 0xCA, 0x00, 0x00, 
                               0x00, 
                               0x00,
                               0x28};
       
      success = nfc.inDataExchange(selectApdu, sizeof(selectApdu), response, &responseLength);
      
      if(success) {
        
        
        DMSG(F("Android response #1: ("));
        DMSG(responseLength);
        DMSG(") ");
        nfc.PrintHexChar(response, responseLength);
        
        
        responseLength = 74;
        memset(response, 0, sizeof(response));
        
        success = nfc.inDataExchange((uint8_t*)&buf, strlen(buf), response, &responseLength);
        
        DMSG(F("Android response #2: ("));
        DMSG(responseLength);
        DMSG(") ");
        nfc.PrintHexChar(response, responseLength);
        
        do {
  
          memset(response, 0, sizeof(response));
          
          success = nfc.inDataExchange(getDataApdu, sizeof(getDataApdu), response, &responseLength);
          
          DMSG(F("Android response #3: ("));
          DMSG(responseLength);
          DMSG(") ");
          nfc.PrintHexChar(response, responseLength);

        
          if(success && responseLength > 2 && response[0] == '^') {
            
            response[strlen((char*)&response)-1] = 0x00;
            
            Serial.println("DONE|" + String((const char*)response));
            
            #if USING_LCD
            clearLcdLine(1);
            lcd.setCursor(0,0);
            lcd.print(F("PAYMENT SUCCESS "));
            lcd.setCursor(0,1);
            lcd.print(F("   THANK YOU    "));
            #endif
  
            DMSG("\nSuccess");
            //get out from loop
            success = false;
          }
          else {
            DMSG(F("Broken connection?\n"));
          }
        }while(success);
      }
      else {
       
        DMSG(F("Failed sending SELECT AID\n"));
      }
    }
    else {
     
      DMSG(F("Didn't find anything!\n"));
    }
  
    delay(1000);
  }
}
  
#endif

#if TEST_TAG
void scanTag(){
  #if USING_LED
  dim_led(12, 1);
  #endif
  
  DMSG("scanning tag...\n");
  if (nfc.tagPresent()){
    
    
    NfcTag tag = nfc.read();
    //if (tag.hasNdefMessage()){
      
      tag.print();
    //}else{
      
      //nfc.format();
    //}
    
    #if USING_LCD
    lcd.setCursor(0,0);
    lcd.print(tag.getTagType());
    lcd.setCursor(0,1);
    String uidString = tag.getUidString();
    uidString.replace(" ","");
    lcd.print(uidString);
    #endif
    #if USING_LED
    dim_led(12, 5);
    #endif
    
    #if TEST_WRITE_TAG
    DMSG("trying to write...\n");
    NdefMessage ndef = NdefMessage();
    
    //char tmp[100];
    //sprintf(tmp, "http://www.ansvia.com?r=%d", random(1, 100000));
    
    //ndef.addUriRecord(tmp);
    ndef.addTextRecord("http://www.activexperts.com/sms-component/smpp-specifications/pdu-type-format-definitions/aaaabbbbbbccccccccdddddddeeeeeeeffffffggggghhhhhiiijjjk");
    if (nfc.write(ndef)){
      DMSG("success writing to tag\n");
    }else{
      MSGERROR("write failed");
    }
    #endif
    
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
  MSGPRINT_printHex((uint8_t)pb[0]);
  MSGPRINT("\n");
  MSGPRINT(F("  Firmware ver. ")); 
  MSGPRINT_DEC(pb[1]);
  MSGPRINT('.'); 
  MSGPRINT_DEC(pb[2]);NL;
  MSGPRINT(F("  Supports ")); 
  MSGPRINT_HEX((uint8_t)pb[3]);NL;
  #endif
}
#endif

#if USING_LED
void dim_led(uint16_t led_num, uint16_t count){
  int i;
  for (i=0;i<count;i++){
    if (i>0)
      delay(50);
    digitalWrite(led_num, HIGH);
    delay(50);
    digitalWrite(led_num, LOW);
  }
}
#endif



