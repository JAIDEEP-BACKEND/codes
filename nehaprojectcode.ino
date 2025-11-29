#include <Servo.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define SS_PIN 10
#define RST_PIN 9

MFRC522 mfrc522(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);

const int buttonPins[4] = {5, 6, 7, 8};
const int servoPins[4] = {A0, 2, 3, 4};

Servo servo;
String authorizedUID = "";

void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Press Button");
  for (int i = 0; i < 4; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }
}

bool waitForCard(String &uid, unsigned long timeoutMs) {
  unsigned long start = millis();
  uid = "";
  while (millis() - start < timeoutMs) {
    if (!mfrc522.PICC_IsNewCardPresent()) {
      continue;
    }
    if (!mfrc522.PICC_ReadCardSerial()) {
      continue;
    }
    uid = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      if (i > 0) {
        uid += " ";
      }
      if (mfrc522.uid.uidByte[i] < 0x10) {
        uid += "0";
      }
      uid += String(mfrc522.uid.uidByte[i], HEX);
    }
    uid.toUpperCase();
    Serial.print("Card UID: ");
    Serial.println(uid);
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    return true;
  }
  return false;
}

void vendItem(int index, const char *priceText) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(priceText);
  lcd.setCursor(0, 1);
  lcd.print("Tap your card");
  String cardUID;
  bool gotCard = waitForCard(cardUID, 10000);
  if (!gotCard) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("No card");
    lcd.setCursor(0, 1);
    lcd.print("Try again");
    delay(1500);
    return;
  }
  if (authorizedUID == "" || cardUID == authorizedUID) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Payment OK");
    lcd.setCursor(0, 1);
    lcd.print("Dispensing");
    if (index >= 0 && index < 4) {
      servo.attach(servoPins[index]);
      servo.write(80);
      delay(2000);
      servo.write(0);
      delay(500);
      servo.detach();
    }
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Take cookies");
    delay(3000);
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Wrong card");
    lcd.setCursor(0, 1);
    lcd.print("Try again");
    delay(3000);
  }
}

void loop() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Select item");
  lcd.setCursor(0, 1);
  lcd.print("A B C D btns");
  if (digitalRead(buttonPins[0]) == LOW) {
    vendItem(0, "5 Rs");
  } else if (digitalRead(buttonPins[1]) == LOW) {
    vendItem(1, "10 Rs");
  } else if (digitalRead(buttonPins[2]) == LOW) {
    vendItem(2, "15 Rs");
  } else if (digitalRead(buttonPins[3]) == LOW) {
    vendItem(3, "20 Rs");
  }
  delay(150);
}
