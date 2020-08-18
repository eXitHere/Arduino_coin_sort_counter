#include <WiFiManager.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <Keypad.h>
#include <EEPROM.h>
#include <HTTPClient.h>
#include <TridentTD_LineNotify.h>

#define coinRead1  15
#define coinRead2  19
#define coinRead5  4
#define coinRead10 5
#define buzzer 2
#define delayPin 34

#define EEPROM_SIZE 10
// 1 2 5 10 target
#define posCoin1    0
#define posCoin2    2
#define posCoin5    4
#define posCoin10   6
#define posTarget   8

#define TokenLine "__TOKEN__"

LiquidCrystal_I2C lcd(0x27, 16, 2);
char keyMap[4][4] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[4] = {13, 12, 14, 27}; //connect to the row pinouts of the keypad
byte colPins[4] = {26, 25, 33, 32}; //connect to the column pinouts of the keypad
Keypad keyPad = Keypad( makeKeymap(keyMap), rowPins, colPins, 4, 4);
Servo myservo;
int state = 0;
/* 0 = แสดงยอดเงิน
   1 =
   2 = ปลดล็อก
*/
String passwordPress = "";
String passwordStar  = "";
int target;
int coinSum;
String coinTemp = "";
int coinCounter1, coinCounter2, coinCounter5, coinCounter10;
int delayCoin = 500;
unsigned long timeStamp = -1;
HTTPClient http;
int tempCoin1 = 0, tempCoin2 = 0, tempCoin5 = 0, tempCoin10 = 0;
unsigned long long lastTime = 0;

void writeIntIntoEEPROM(int address, int number) {
  beep(50);
  EEPROM.write(address, number >> 8);
  EEPROM.write(address + 1, number & 0xFF);
  EEPROM.commit();
}

int readIntFromEEPROM(int address) {
  byte byte1 = EEPROM.read(address);
  byte byte2 = EEPROM.read(address + 1);
  return (byte1 << 8) + byte2;
}

//นับเหรียญ 1
void countCoun1() {
  if(millis() - lastTime > delayCoin) {
    coinCounter1++;
    coinSum += 1;
    writeIntIntoEEPROM(posCoin1, coinCounter1);
    timeStamp = millis();
    tempCoin1++;
    lastTime = millis();
  }
}
//นับเหรียญ 2
void countCoun2() {
  if(millis() - lastTime > delayCoin) {
    coinCounter2++;
    coinSum += 2;
    writeIntIntoEEPROM(posCoin2, coinCounter2);
    timeStamp = millis();
    tempCoin2++;
    lastTime = millis();
  }
}
//นับเหรียญ 5
void countCoun5() {
  if(millis() - lastTime > delayCoin) {
    coinCounter5++;
    coinSum += 5;
    writeIntIntoEEPROM(posCoin5, coinCounter5);
    timeStamp = millis();
    tempCoin5++;
    lastTime = millis();
  }
}
//นับเหรียญ 10
void countCoun10() {
  if(millis() - lastTime > delayCoin) {
    coinCounter10++;
    coinSum += 10;
    writeIntIntoEEPROM(posCoin10, coinCounter10);
    timeStamp = millis();
    tempCoin10++;
    lastTime = millis();
  }
}

void resetEEPROM() {
  writeIntIntoEEPROM(posCoin1, 0);
  writeIntIntoEEPROM(posCoin2, 0);
  writeIntIntoEEPROM(posCoin5, 0);
  writeIntIntoEEPROM(posCoin10, 0);
  writeIntIntoEEPROM(posTarget, 0);
}

int getDelayTime() {
  long temp = 0;
  for (int i = 0; i < 20; i++) {
    delay(10);
    temp += map(analogRead(delayPin), 0, 4095, 1, 1500); // วัดค่าจาก VR เอามาเป็นค่า delay
  }
  return temp / 20;
}

void setup() {
  // EEPROM
  EEPROM.begin(EEPROM_SIZE);
  coinCounter1  = readIntFromEEPROM(posCoin1);
  coinCounter2  = readIntFromEEPROM(posCoin2);
  coinCounter5  = readIntFromEEPROM(posCoin5);
  coinCounter10 = readIntFromEEPROM(posCoin10);
  target        = readIntFromEEPROM(posTarget);
  coinSum       = (coinCounter10 * 10) + (coinCounter5 * 5) + (coinCounter2 * 2) + (coinCounter1 * 1);
  // Setup board
  Serial.begin(115200);
  pinMode(coinRead1,  INPUT);
  pinMode(coinRead2,  INPUT);
  pinMode(coinRead5,  INPUT);
  pinMode(coinRead10, INPUT);
  attachInterrupt(digitalPinToInterrupt(coinRead1), countCoun1, FALLING );
  attachInterrupt(digitalPinToInterrupt(coinRead2), countCoun2, FALLING );
  attachInterrupt(digitalPinToInterrupt(coinRead5), countCoun5, FALLING );
  attachInterrupt(digitalPinToInterrupt(coinRead10), countCoun10, FALLING );
  pinMode(buzzer, OUTPUT);
  pinMode(delayPin, INPUT);
  myservo.attach(18);
  myservo.write(0);
  // LCD Setting
  lcd.init();
  lcd.backlight();
  lcd.print("Wait for wifi");

  // Setup Wifi
  WiFi.mode(WIFI_STA);
  WiFiManager wm;
  //wm.resetSettings();
  bool res;
  res = wm.autoConnect("AutoConnectAP", "1212312121"); // password protected ap

  if (!res) {
    Serial.println("Failed to connect");
    ESP.restart();
  }
  else {
    delay(10);
    delayCoin = getDelayTime();
    lcd.clear();
    lcd.print("Delay time");
    lcd.setCursor(0, 1);
    lcd.print(delayCoin);
    delay(1000);
    beep(50);
    beep(200);
    lcd.clear();
    lcd.print("Local IP");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP());
    delay(2000);
  }

  /*
     init sceen
  */
  lcd.setCursor(0, 0);
  lcd.printf("M: %d/%d", coinSum, target);
  lcd.setCursor(0, 1);
  lcd.printf("%3s %3s %3s %3s", String(coinCounter1), String(coinCounter2), String(coinCounter5), String(coinCounter10));

  /*
     Test post api
  */
  String serverName = "https://script.google.com/macros/s/____SOME_WEB_APP____";
  http.begin(serverName);

  // Specify content-type header
  http.addHeader("Content-Type", "application/json");

  // กำหนด Line Token
  LINE.setToken(TokenLine);

  // ตัวอย่างส่งข้อความ
  LINE.notify("เปิดเครื่องแล้ว");
}

void beep(int _delay) {
  digitalWrite(buzzer, HIGH);
  delay(_delay);
  digitalWrite(buzzer, LOW);
  delay(_delay);
}

void clearState() {
  passwordPress = "";
  passwordStar  = "";
  coinTemp      = "";
}

void notification() {
  if(millis()-timeStamp>10000 and timeStamp != -1) {
    timeStamp  = -1;
    LINE.notify("ยอดเงินที่ต้องการออม " + String(target) + " บาท\nยอดเงินที่ออมได้แล้ว " + String(coinSum) + " บาท" + "\ncoin1: " + String(coinCounter1) + "\ncoin2: " + String(coinCounter2) + "\ncoin5: " + String(coinCounter5) + "\ncoin10: " + String(coinCounter10));
    // Data to send with HTTP POST
    String httpRequestData = "{\"coin1\": " + String(tempCoin1) + ",\"coin2\": " + String(tempCoin2) + ",\"coin5\": " + String(tempCoin5) + ",\"coin10\": " + String(tempCoin10) + "}";
    // Send HTTP POST request
    int httpResponseCode = http.POST(httpRequestData);
    tempCoin1  = 0;
    tempCoin2  = 0;
    tempCoin5  = 0;
    tempCoin10 = 0;
  }
}

void loop() {
  Serial.println(digitalRead(coinRead2));
  notification();
  /*
  if (digitalRead(coinRead1) == 0) {
    coinCounter1++;
    coinSum += 1;
    writeIntIntoEEPROM(posCoin1, coinCounter1);
    timeStamp = millis();
    tempCoin1++;
    delay(delayCoin);
  }
  else if (digitalRead(coinRead2) == 0) {
    coinCounter2++;
    coinSum += 2;
    writeIntIntoEEPROM(posCoin2, coinCounter2);
    timeStamp = millis();
    tempCoin2++;
    delay(delayCoin);
  }
  else if (digitalRead(coinRead5) == 0) {
    coinCounter5++;
    coinSum += 5;
    writeIntIntoEEPROM(posCoin5, coinCounter5);
    timeStamp = millis();
    tempCoin5++;
    delay(delayCoin);
  }
  else if (digitalRead(coinRead10) == 0) {
    coinCounter10++;
    coinSum += 10;
    writeIntIntoEEPROM(posCoin10, coinCounter10);
    timeStamp = millis();
    tempCoin10++;
    delay(delayCoin);
  }
  */
  char keyPress = keyPad.getKey();
  if (keyPress) {
    beep(30);
    if (keyPress == 'A') {
      //ปลดล็อค
      state = 2;
      lcd.setCursor(0, 0);
      lcd.print("Unlock mode     ");
      lcd.setCursor(0, 1);
      lcd.print("                ");
      clearState();
    }
    else if (keyPress == 'B') {
      //หยอดเงินปกติ
      state = 0;
      clearState();
    }
    else if (keyPress == 'C') {
      state = 1;
      lcd.setCursor(0, 0);
      lcd.print("CHANGE MONEY    ");
      lcd.setCursor(0, 1);
      lcd.print("                ");
      clearState();
    }
    else if (keyPress == 'D') {
      state = 3;
      lcd.setCursor(0, 0);
      lcd.print("RESET MEMORY    ");
      lcd.setCursor(0, 1);
      lcd.print("                ");
    }
  }

  if ( state == 0 ) {
    lcd.setCursor(0, 0);
    lcd.printf("M: %d/%d             ", coinSum, target);
    lcd.setCursor(0, 1);
    lcd.printf("%3s %3s %3s %3s", String(coinCounter1), String(coinCounter2), String(coinCounter5), String(coinCounter10));
  }
  else if ( state == 1 ) {
    if (keyPress != 'A' and keyPress != 'B' and keyPress != 'C' and keyPress != 'D' and keyPress != '#' and keyPress != '*' and keyPress) {
      if (coinTemp.length() < 6) {
        coinTemp += keyPress;
      }
      lcd.setCursor(0, 1);
      lcd.print(coinTemp);
    }
    else if (keyPress == '#') {
      state = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      target = coinTemp.toInt();
      writeIntIntoEEPROM(posTarget, target); // เปลี่ยนค่าเป้าหมายการออมใน EEPROM
      EEPROM.commit();               // บันทึก
      lcd.print("Saved!");
    }
  }
  else if ( state == 2 ) { // ปลดล็อคตู้
    if (keyPress != 'A' and keyPress != 'B' and keyPress != 'C' and keyPress != 'D' and keyPress != '#' and keyPress != '*' and keyPress) {
      passwordPress += keyPress;
      passwordStar  += '*';
      lcd.setCursor(0, 1);
      lcd.print(passwordStar);
      if (passwordPress == "123456") {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Unlocked!");
        myservo.write(90);
        LINE.notify("เปิดกระปุกแล้ว");
      }
    }
    else if (keyPress == '#') {
      state = 0;
      myservo.write(0);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("locked!");
      LINE.notify("ปิดกระปุกแล้ว");
    }
  }
  else if(state == 3) { // reset ตู้
    if (keyPress != 'A' and keyPress != 'B' and keyPress != 'C' and keyPress != 'D' and keyPress != '#' and keyPress != '*' and keyPress) {
        passwordPress += keyPress;
        passwordStar  += '*';
        lcd.setCursor(0, 1);
        lcd.print(passwordStar);
        if (passwordPress == "123456") {
          lcd.setCursor(0, 0);
          lcd.print("CEAR MEMORY...    ");
          lcd.setCursor(0, 1);
          lcd.print("                ");
          resetEEPROM();
          beep(20);
          beep(20);
          beep(20);
          LINE.notify("เคลียร์ค่ากระปุกออมสินแล้ว");
          delay(1000);
          lcd.setCursor(0, 0);
          lcd.print("RESTARTING...  ");
          delay(2000);
          ESP.restart();
        }
      }
  }
}
