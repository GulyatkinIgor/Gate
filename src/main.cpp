#include <GyverPortal.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

struct LoginPass {
  char ssid[20];
  char pass[20];
  char tokk[40];
  char serv[20];
};

WidgetLED VPIN_gateCheckLamp (V10);
WidgetLED VPIN_gateLamp (V11);
WidgetLED VPIN_doorLamp (V12);
WidgetLED VPIN_gateLightInd (V13);
WidgetLED VPIN_roadLightInd (V14);
WidgetLED VPIN_blockButtonInd (V15);
WidgetLED VPIN_blockGateInd (V16);

#define VPIN_openGate V5
#define VPIN_openDoor V6
#define VPIN_gateLight V7
#define VPIN_roadLight V8
#define VPIN_stateGate V9
#define VPIN_blockButton V20
#define VPIN_blockGate V21

#define chekLamp D0
#define indicate D4
#define gate D5
#define door D6
#define gateLight D7
#define roadLight D8

uint32_t myTimer;
uint32_t myTimer2;
int checkPeriod = 1000;
int readPeriod = 3000;
float blockGateFloat;

bool stateGateLight;
bool stateGateLightFlag;
bool stateRoadLight;
bool stateRoadLightFlag;
bool blockButton;
bool blockButtonFlag;
bool blockGate;
bool blockGateFlag;
bool openGate;
bool openDoor;

LoginPass lp;

void build() {
  GP.BUILD_BEGIN();
  GP.PAGE_TITLE("Gate config control");
  GP.THEME(GP_DARK);
  GP.TITLE("Настройка параметров подключения контроллера ворот");
  GP.FORM_BEGIN("/login");
  GP.TEXT("lg", "Login", lp.ssid);
  GP.BREAK();
  GP.TEXT("ps", "Password", lp.pass);
  GP.BREAK();
  GP.TEXT("tk", "Tokken", lp.tokk);
  GP.BREAK();
  GP.TEXT("sv", "Server", lp.serv);
  GP.SUBMIT("Submit");
  GP.FORM_END();
  GP.BUILD_END();
}

void connectWiFi() {
  Serial.print("Connect to: ");
  Serial.println(lp.ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(lp.ssid, lp.pass);
  int n = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    n++;
    if (n >= 60) {
      n = 0;
      Serial.println();
      Serial.print("Not connected! Start AP ");
      loginPortal();
    }
  }
  Serial.println();
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  Serial.print("AP: ");
  Serial.println(lp.ssid);
}

void connectBlynk() {
  int8_t count = 0;
  while (!Blynk.connected()) {
    Serial.println();
    Serial.print("Connect to server blynk...");
    IPAddress serverIp;
    if (serverIp.fromString(lp.serv)) {
      Serial.println(serverIp);
    }
    Blynk.config(lp.tokk, serverIp, 8080);
    Blynk.connect();
    Serial.print(".");
    delay(1000);
    if (count >= 30) {
      Serial.println();
      Serial.print("No respone...");
      Serial.println();
      Serial.print("restart ESP");
      count = 0;
      delay(2000);
      ESP.restart();
    }
    count ++;
  }
  Serial.println();
  Serial.print("Blynk CONECTED!");
}

void setup() {
  pinMode(gate, OUTPUT);
  pinMode(door, OUTPUT);
  pinMode(gateLight, OUTPUT);
  pinMode(roadLight, OUTPUT);
  pinMode(indicate, OUTPUT);
  digitalWrite(indicate, HIGH);
  delay(2000);
  Serial.begin(115200);
  Serial.println();
  // читаем логин пароль из памяти
  EEPROM.begin(200);
  EEPROM.get(0, lp);
  connectWiFi();
  connectBlynk();
}



void loginPortal() {
  Serial.println("Portal start");
  digitalWrite(indicate, LOW);
  // запускаем точку доступа
  WiFi.mode(WIFI_AP);
  WiFi.softAP("WiFi Config");
  delay(2000);
  // запускаем портал
  GyverPortal ui;
  ui.attachBuild(build);
  ui.start(WIFI_AP);
  ui.attach(action);
  while (ui.tick());
}

void action(GyverPortal& p) {
  if (p.form("/login")) {      // кнопка нажата
    p.copyStr("lg", lp.ssid);  // копируем себе
    p.copyStr("ps", lp.pass);
    p.copyStr("tk", lp.tokk);
    p.copyStr("sv", lp.serv);
    EEPROM.put(0, lp);              // сохраняем
    EEPROM.commit();                // записываем
    WiFi.softAPdisconnect();        // отключаем AP
    digitalWrite(indicate, HIGH);
    delay(3000);
    Serial.println();
    Serial.print("Restart ESP...");
    ESP.restart();
  }
}

void checkGate() {
  if (millis() - myTimer >= checkPeriod) {
    myTimer += checkPeriod;
    if (digitalRead(chekLamp) == HIGH) {
      VPIN_gateCheckLamp.off();
      Blynk.virtualWrite(VPIN_stateGate, "false");
    } else if (digitalRead(chekLamp) == LOW) {
      Blynk.virtualWrite(VPIN_stateGate, "true");
      VPIN_gateCheckLamp.on();
    }
  }
}

void chekButtons() {
  //BLOCK GATE
  if (blockGateFlag == HIGH) {
    blockGateFunc();
  }
  //BLOCK BUTTON
  if (blockButton == HIGH) {
    blockButtonFunc();
  }
  //OPEN GATE
  if (openGate == HIGH) {
    openGateFunc();
  }
  //OPEN DOOR
  if (openDoor == HIGH) {
    openDoorFunc();
  }
  //GATE LIGHT
  if (stateGateLightFlag == HIGH){
    gateLightFunc();
  }
  //ROAD LIGHT
  if (stateRoadLightFlag == HIGH){
    roadLightFunc();
  }
}


BLYNK_WRITE(VPIN_blockGate) {
  blockGateFloat = param.asFloat();
  blockGateFlag = HIGH;
}

BLYNK_WRITE(VPIN_blockButton) {
  blockButton = param.asInt();
  blockButtonFlag = HIGH;
}

BLYNK_WRITE(VPIN_openGate) {
  openGate = param.asInt();
}

BLYNK_WRITE(VPIN_openDoor) {
  openDoor = param.asInt();
}

BLYNK_WRITE(VPIN_gateLight) {
  stateGateLight = param.asInt();
  stateGateLightFlag = HIGH;
}

BLYNK_WRITE(VPIN_roadLight) {
  stateRoadLight = param.asInt();
  stateRoadLightFlag = HIGH;
}

void roadLightFunc(){
  if (stateRoadLight == HIGH) {
    digitalWrite(roadLight, HIGH);
    digitalWrite(indicate, LOW);
    delay(100);
    digitalWrite(indicate, HIGH);
    delay(100);
    digitalWrite(indicate, LOW);
    delay(100);
    digitalWrite(indicate, HIGH);
    delay(100);
    digitalWrite(indicate, LOW);
    delay(100);
    digitalWrite(indicate, HIGH);
    Serial.println();
    Serial.print("Road Light On!");
    stateRoadLightFlag = LOW;
    VPIN_roadLightInd.on();
  }
  if (stateRoadLight == LOW) {
    digitalWrite(roadLight, LOW);
    digitalWrite(indicate, LOW);
    delay(100);
    digitalWrite(indicate, HIGH);
    delay(100);
    digitalWrite(indicate, LOW);
    delay(100);
    digitalWrite(indicate, HIGH);
    delay(100);
    digitalWrite(indicate, LOW);
    delay(100);
    digitalWrite(indicate, HIGH);
    Serial.println();
    Serial.print("Road Light Off!");
    stateRoadLightFlag = LOW;
    VPIN_roadLightInd.off();
  }
}

void gateLightFunc(){
  if (stateGateLight == HIGH) {
    digitalWrite(gateLight, HIGH);
    digitalWrite(indicate, LOW);
    delay(100);
    digitalWrite(indicate, HIGH);
    delay(100);
    digitalWrite(indicate, LOW);
    delay(100);
    digitalWrite(indicate, HIGH);
    delay(100);
    digitalWrite(indicate, LOW);
    delay(100);
    digitalWrite(indicate, HIGH);
    Serial.println();
    Serial.print("Gate Light On!");
    stateGateLightFlag = LOW;
    VPIN_gateLightInd.on();
  }
  if (stateGateLight == LOW) {
    digitalWrite(gateLight, LOW);
    digitalWrite(indicate, LOW);
    delay(100);
    digitalWrite(indicate, HIGH);
    delay(100);
    digitalWrite(indicate, LOW);
    delay(100);
    digitalWrite(indicate, HIGH);
    delay(100);
    digitalWrite(indicate, LOW);
    delay(100);
    digitalWrite(indicate, HIGH);
    Serial.println();
    Serial.print("Gate Light Off!");
    stateGateLightFlag = LOW;
    VPIN_gateLightInd.off();
  }
}

void blockGateFunc () {
  if (blockGate == LOW and blockGateFlag == HIGH) {
    if (blockGateFloat < 1) {
      Serial.println();
      Serial.print("ERROR LOCK!");
      Blynk.virtualWrite(VPIN_blockGate, 0);
      VPIN_blockGateInd.off();
      blockGateFlag = LOW;
    } else if ( blockGateFloat == 1) {
      Serial.println();
      Serial.print("LOCK!");
      Blynk.virtualWrite(VPIN_blockGate, 1);
      blockGate = HIGH;
      VPIN_blockGateInd.on();
      blockGateFlag = LOW;
    }
  }
  if (blockGate == HIGH and blockGateFlag == HIGH) {
    if (blockGateFloat > 0) {
      Serial.println();
      Serial.print("ERROR UNLOCK!");
      Blynk.virtualWrite(VPIN_blockGate, 1);
      VPIN_blockGateInd.on();
      blockGateFlag = LOW;
    } else if ( blockGateFloat == 0) {
      Serial.println();
      Serial.print("UNLOCK!");
      Blynk.virtualWrite(VPIN_blockGate, 0);
      blockGate = LOW;
      VPIN_blockGateInd.off();
      blockGateFlag = LOW;
    }
  }
}

void blockButtonFunc() {
  if (blockGate == LOW) {
    VPIN_blockButtonInd.off();
    if (blockButtonFlag == HIGH) {
      myTimer2 = millis();
      Serial.println();
      Serial.print("UNLOCK!");
      blockButtonFlag = LOW;
    }
    if (millis() - myTimer2 >= readPeriod) {
      VPIN_blockButtonInd.on();
      blockButton = LOW;
      Blynk.virtualWrite(VPIN_blockButton, LOW);
      Serial.println();
      Serial.print("LOCK!");
    }
  } else {
    Serial.println();
    Serial.print("ERROR!");
    VPIN_blockGateInd.off();
    delay(100);
    VPIN_blockGateInd.on();
    delay(100);
    VPIN_blockGateInd.off();
    delay(100);
    VPIN_blockGateInd.on();
    delay(100);
    VPIN_blockGateInd.off();
    delay(100);
    VPIN_blockGateInd.on();
    blockButton = LOW;
    Blynk.virtualWrite(VPIN_blockButton, LOW);
  }
}

void openGateFunc() {
  if (blockButton == HIGH and blockGate == LOW) {
    VPIN_gateLamp.on();
    Serial.println();
    digitalWrite(indicate, LOW);
    Serial.print("OPEN GATE");
    digitalWrite(gate, HIGH);
    delay(1000);
    digitalWrite(gate, LOW);
    Serial.println();
    digitalWrite(indicate, HIGH);
    Serial.print("DONE!");
    VPIN_gateLamp.off();
    Blynk.virtualWrite(VPIN_openGate, LOW);
    blockButton = LOW;
    Blynk.virtualWrite(VPIN_blockButton, LOW);
    Serial.println();
    Serial.print("LOCK!");
    openGate = LOW;
    VPIN_blockButtonInd.on();
  } else {
    Serial.println();
    Serial.print("ERROR!");
    VPIN_blockButtonInd.off();
    delay(100);
    VPIN_blockButtonInd.on();
    delay(100);
    VPIN_blockButtonInd.off();
    delay(100);
    VPIN_blockButtonInd.on();
    delay(100);
    VPIN_blockButtonInd.off();
    delay(100);
    VPIN_blockButtonInd.on();
    openGate = LOW;
    Blynk.virtualWrite(VPIN_openGate, LOW);
  }
}

void openDoorFunc(){
    if (blockButton == HIGH and blockGate == LOW) {
      VPIN_doorLamp.on();
      Serial.println();
      digitalWrite(indicate, LOW);
      Serial.print("OPEN DOOR");
      digitalWrite(door, HIGH);
      delay(1000);
      digitalWrite(door, LOW);
      Serial.println();
      digitalWrite(indicate, HIGH);
      Serial.print("DONE!");
      VPIN_doorLamp.off();
      Blynk.virtualWrite(VPIN_openDoor, LOW);
      blockButton = LOW;
      Blynk.virtualWrite(VPIN_blockButton, LOW);
      Serial.println();
      Serial.print("LOCK!");
      openDoor = LOW;
      VPIN_blockButtonInd.on();
    } else {
      Serial.println();
      Serial.print("ERROR!");
      VPIN_blockButtonInd.off();
      delay(100);
      VPIN_blockButtonInd.on();
      delay(100);
      VPIN_blockButtonInd.off();
      delay(100);
      VPIN_blockButtonInd.on();
      delay(100);
      VPIN_blockButtonInd.off();
      delay(100);
      VPIN_blockButtonInd.on();
      openDoor = LOW;
      Blynk.virtualWrite(VPIN_openDoor, LOW);
    }
}


void loop() {
  checkGate();
  chekButtons();
  Blynk.run();
}