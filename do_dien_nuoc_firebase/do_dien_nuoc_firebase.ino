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
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// ===== PZEM =====
PZEM004Tv30 pzem(D5, D6);

// ===== FLOW SENSOR =====
#define FLOW_PIN D2
volatile unsigned long pulseCount = 0;

float flowRate = 0.0;
float totalMilliLitres = 0.0;
unsigned long lastFlowCalc = 0;

const float calibrationFactor = 450.0;

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

  Firebase.begin(&config, &auth);
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

    flowRate = (count / calibrationFactor) * 1000.0;
    totalMilliLitres += flowRate;
    lastFlowCalc = now;

    float voltage   = pzem.voltage();
    float current   = pzem.current();
    float power     = pzem.power();
    float energy    = pzem.energy();
    float frequency = pzem.frequency();

    float totalM3 = totalMilliLitres / 1000000.0;

    String uid = auth.token.uid.c_str();
    String path = "/users/" + uid;

    Firebase.setFloat(fbdo, path + "/voltage", voltage);
    Firebase.setFloat(fbdo, path + "/current", current);
    Firebase.setFloat(fbdo, path + "/power", power);
    Firebase.setFloat(fbdo, path + "/energy", energy);
    Firebase.setFloat(fbdo, path + "/frequency", frequency);

    Firebase.setFloat(fbdo, path + "/flow_ml_s", flowRate);
    Firebase.setFloat(fbdo, path + "/water_m3", totalM3);
    Firebase.setInt(fbdo, path + "/timestamp", now / 1000);

    Serial.println("Da gui Firebase");
  }
}
