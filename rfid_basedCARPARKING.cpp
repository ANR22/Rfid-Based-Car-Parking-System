// Arduino Car Parking System

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include <Servo.h>
#include <SPI.h>
#include <MFRC522.h>
#define SS_PIN 10
#define RST_PIN 9

Servo myservo1;
LiquidCrystal_I2C lcd(0x27, 16, 2);
MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;


const int IR1 = 4;
const int IR2 = 2;
const int IR3 = 3;
const int buttonPin = 7;
int buttonState = 0;
int slotA;
int IR[3] = {IR1, IR2, IR3};


void setup() {
  
  SPI.begin();
  Serial.begin(9600);
  while (!Serial);
  
  mfrc522.PCD_Init();
  
  lcd.init();
  lcd.backlight();

  pinMode(IR1, INPUT);
  pinMode(IR2, INPUT);
  pinMode(buttonPin, INPUT);

  myservo1.attach(6);
  myservo1.write(180);

  lcd.setCursor (0, 0);
  lcd.print(" RFID BASED CAR ");
  lcd.setCursor (0, 1);
  lcd.print(" PARKING SYSTEM ");
  delay (2000);
  lcd.clear();

//  Serial.println("waiting for card...");
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

}

void loop() {

  byte trailerBlock   = 7;
  MFRC522::StatusCode status;
  byte buffer[18];
  byte size = sizeof(buffer);
  byte blockAddr = 4;
  byte readData[18];
  slotA = 0;
//  for (int i = 0; i < 3; i++) {
//    if (digitalRead(IR[i]) == HIGH )
//      slotA++;
//  }
//  
//  if (slotA == 0) {
//    Serial.println("no slots available ");
//    delay(500);
//    lcd.setCursor (0, 0);
//    lcd.print("   SLOTS FULL    ");
//    delay (2000);
//    lcd.clear();
//    myservo1.write(0);
//    return;
//  }
    lcd_display();


  if ( ! mfrc522.PICC_IsNewCardPresent())
    return;

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial())
    return;


  String strID = "";
  for (byte i = 0; i < 4; i++) {
    strID +=
      (mfrc522.uid.uidByte[i] < 0x10 ? "0" : "") +
      String(mfrc522.uid.uidByte[i], HEX) +
      (i != 3 ? ":" : "");
  }
  strID.toUpperCase();


  delay(500);


  if (strID.indexOf("2C:7A:FA:37") >= 0 || strID.indexOf("19:85:F7:E2") >= 0 || strID.indexOf("03:C8:B3:15") >= 0 || strID.indexOf("3A:E2:96:7F") >= 0 ) {

    Serial.println("*Authorised acces*");

    Serial.println(F("Authenticating using key A..."));
    status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
      Serial.print(F("PCD_Authenticate() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
      return;
    }
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
      Serial.print(F("MIFARE_Read() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
    }


    // Authenticate using key B
    Serial.println(F("Authenticating again using key B..."));
    status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, trailerBlock, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
      Serial.print(F("PCD_Authenticate() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
      return;
    }
    Serial.println("read successful");
    for (byte i = 0; i < 16; i++) {

      readData[i] = buffer[i];


    }
    if (readData[2] == 0) {
      long int t1;
      t1 = millis();
      int minutes = (t1 / 1000);
      readData[1] = minutes;
      readData[2] = 1;


      /* Authenticating the desired data block for write access using Key A */
      // Write data to the block
      Serial.print(F("Writing data into block ")); Serial.print(blockAddr);
      Serial.println(F(" ..."));

      status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(blockAddr, readData, 16);
      if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Write() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
      }
      Serial.println("write successful");
      lcd.clear();
      lcd.setCursor (0, 0);
      lcd.print("!!WELCOME!!");
      delay(2000);
      lcd.clear();
      lcd.print("BALANCE:");
      lcd.setCursor(0,1);
      lcd.print(readData[0]);
//      lcd.setCursor (0, 1);
//      lcd.print("SLOTS -  ");
//      lcd.print(slotA);
      delay (2000);
      lcd.clear();
      myservo1.write(100);
      delay (3000);
      myservo1.write(180);
      lcd_display();
      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();


    }
    else if (readData[2] == 1) {
      long int t1 = readData[1];
      long int t2 = millis();
      t2 = t2 / 1000;
      int timespan = t2 - t1-10;
      int mins = timespan;
      
      
      //      assuming 1min 1 rupees
      int fee = 30+mins;
//    int fee = 150;
      lcd.clear();
      lcd.setCursor (0, 0);
      lcd.print("Duration: ");
      lcd.print(mins);
      lcd.setCursor (0, 1);
      lcd.print("CHARGES: ");
      lcd.print(fee);
      delay (2000);
      while(readData[0] < fee) {
        lcd.clear();
        lcd.setCursor (0, 0);
        lcd.print(" LOW BALANCE ");
        lcd.setCursor(0, 1);
        lcd.print("Press the button");

        readData[0] = recharge(readData[0]);
//        lcd.clear();
//        lcd.print("Recharge Done");
      }
      int bal = readData[0] - fee;
      readData[0] = bal;
      readData[1] = 0;
      readData[2] = 0;
//      lcd.clear();
//      lcd.print(readData[2]);
      /* Authenticating the desired data block for write access using Key A */
      // Write data to the block
      Serial.print(F("Writing data into block ")); Serial.print(blockAddr);
      Serial.println(F(" ..."));

      status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(blockAddr, readData, 16);
      if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Write() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
      }
      lcd.clear();
      lcd.print("BALANCE: ");
      lcd.print(readData[0]);
      delay(2000);
      lcd.clear();
      lcd.print("THANK YOU");
      myservo1.write(100);
      delay (3000);
      lcd.clear();
      myservo1.write(180);
      Serial.println("write successful");

      lcd_display();
      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();

    }
  }
  else
  {

    Serial.println("Acces denied");
    lcd.setCursor (0, 0);
    lcd.print("  ACCESS DENIED    ");
    delay (2000);
    lcd.clear();
  }

}

int recharge(int balance) {
  buttonState = digitalRead(buttonPin);
  if (buttonState == HIGH) {
    lcd.clear();
    balance += 100;
    lcd.clear();
    lcd.print("Recharge Successful");
    delay(2000);
    lcd.clear();
    lcd.print("New Balance: ");
    lcd.setCursor(0, 1);
    lcd.print(balance);
    delay(2000);
    lcd.clear();

    return balance;
  }
  recharge(balance);

}

void lcd_display(){
  lcd.clear();
//  delay(1000);
  for (int i = 0; i < 3; i++) {
    if(i==0){
      lcd.setCursor(0,0);
    }
    else if(i == 1){
      lcd.setCursor(9,0);
    }
    else{
      lcd.setCursor(4,1);
    }
    lcd.print("SLOT");
    lcd.print(i+1);
    lcd.print(":");
    if (digitalRead(IR[i]) == HIGH ){
       slotA++;
       lcd.print("O");
    }
    else{
      lcd.print("X");
    }
//    delay(1000);
  }
  delay(1000);
  if (slotA == 0) {
    Serial.println("no slots available ");
    delay(500);
    lcd.clear();
    lcd.setCursor (0, 0);
    lcd.print("   SLOTS FULL    ");
    delay (2000);
    lcd.clear();
//    myservo1.write();
    return;
  }
}
