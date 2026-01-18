/***************************************************
 * ESP8266 – ĐO ĐIỆN + ĐO NƯỚC
 * FIREBASE – LOGIN USER TẠO SẴN
 ***************************************************/

#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <PZEM004Tv30.h>

// ===== WIFI =====
#define WIFI_SSID     "Tien Phuoc"
#define WIFI_PASSWORD "thienlyoi"

// ===== FIREBASE =====
#define FIREBASE_API_KEY "AIzaSyD9Vmq5O9sVpOsTtQ3qNIyggsIaobNfBMw"
#define FIREBASE_DATABASE_URL "https://do-an-high-default-rtdb.firebaseio.com"

// ===== AUTH (USER TẠO THỦ CÔNG TRÊN FIREBASE) =====
#define USER_EMAIL    "nhanle@gmail.com"
#define USER_PASSWORD "nhanle0712"

// ===== FIREBASE OBJECT =====
FirebaseData fbdo; //gui-nhan du lieu
FirebaseAuth auth; //chua thong tin dang nhap
FirebaseConfig config; // chua API key, URl

// ===== PZEM =====
PZEM004Tv30 pzem(D5, D6); //D5-RX, D6-TX

// ===== FLOW SENSOR =====
#define FLOW_PIN D2
volatile unsigned long pulseCount = 0;

float flowRate = 0.0; //luu luong nuoc
float totalMilliLitres = 0.0; //tong nuoc da chay
unsigned long lastFlowCalc = 0; //thoi diem tinh gan nhat

const float calibrationFactor = 450.0; //he so hieu chinh YF-S201-450 xung = 1 lit nuoc

// ===== INTERRUPT =====
void ICACHE_RAM_ATTR countPulse() {
  pulseCount++;
}

// ============================
// SETUP
// ============================
void setup() {
  Serial.begin(115200);

  pinMode(FLOW_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FLOW_PIN), countPulse, RISING);
//ngat khi xung len muc cao (rising)
  // ---- WIFI ----
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Dang ket noi WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi OK");

  // ---- FIREBASE CONFIG ----
  config.api_key = FIREBASE_API_KEY;
  config.database_url = FIREBASE_DATABASE_URL;

  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  Firebase.begin(&config, &auth);//khoi dong firebase
  Firebase.reconnectWiFi(true);

  // ---- ĐỢI LOGIN THÀNH CÔNG ----
  Serial.print("Dang dang nhap Firebase");
  while (!auth.token.uid.length()) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nFirebase login OK");
  Serial.print("UID: ");
  Serial.println(auth.token.uid.c_str());
}

// ============================
// LOOP
// ============================
void loop() {
  unsigned long now = millis();

  if (now - lastFlowCalc >= 1000) {
    noInterrupts();
    unsigned long count = pulseCount;
    pulseCount = 0;
    interrupts();
//tinh luu luong moi giay
    flowRate = (count / calibrationFactor) * 1000.0;
    totalMilliLitres += flowRate;
    lastFlowCalc = now;
//tinh luu luong hien tai
    float voltage   = pzem.voltage();
    float current   = pzem.current();
    float power     = pzem.power();
    float energy    = pzem.energy();
    float frequency = pzem.frequency();
//do du lieu PZEM
    float totalM3 = totalMilliLitres / 1000000.0;
//doi don vi tu ml->m3
    String uid = auth.token.uid.c_str();
    String path = "/users/" + uid;
//moi user co du lieu rieng
    Firebase.setFloat(fbdo, path + "/voltage", voltage);
    Firebase.setFloat(fbdo, path + "/current", current);
    Firebase.setFloat(fbdo, path + "/power", power);
    Firebase.setFloat(fbdo, path + "/energy", energy);
    Firebase.setFloat(fbdo, path + "/frequency", frequency);

    Firebase.setFloat(fbdo, path + "/flow_ml_s", flowRate);
    Firebase.setFloat(fbdo, path + "/water_m3", totalM3);
    Firebase.setInt(fbdo, path + "/timestamp", now / 1000);
//gui du lieu len firebase
    Serial.println("Da gui Firebase");
  }
}
