#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

#define SERVICE_UUID        "12345678-1234-1234-1234-1234567890ab"
#define CHARACTERISTIC_UUID "abcdefab-1234-5678-1234-abcdefabcdef"

#define IGNITION_PIN 4
#define START_PIN    5
#define SEAT_PIN     6
#define KILL_PIN     7
#define BATTERY_PIN  1

const bool RELAY_ACTIVE_LOW = false;

#define R1 100000.0
#define R2 27000.0

bool ignitionOn = false;
bool deviceConnected = false;
bool starterActive = false;
bool seatActive = false;

unsigned long starterStartTime = 0;
unsigned long lastStarterStopTime = 0;
unsigned long seatStartTime = 0;
unsigned long disconnectTime = 0;
unsigned long lastStatusTime = 0;

bool restartAdvertisingPending = false;

const unsigned long STARTER_DURATION = 1000;
const unsigned long STARTER_COOLDOWN = 1500;
const unsigned long SEAT_DURATION = 500;
const unsigned long IGNITION_SWITCH_DELAY = 75;
const unsigned long ADVERTISING_RESTART_DELAY = 300;
const unsigned long STATUS_INTERVAL = 2000;

BLECharacteristic *commandCharacteristic = nullptr;

float readBatteryVoltage();
void sendStatus();
void setRelay(uint8_t pin, bool active);
void ignitionRelay(bool active);
void killRelay(bool active);
void starterRelay(bool active);
void seatRelay(bool active);
void ignitionOnSequence();
void ignitionOffSequence();

void setRelay(uint8_t pin, bool active) {
  if (RELAY_ACTIVE_LOW) {
    digitalWrite(pin, active ? LOW : HIGH);
  } else {
    digitalWrite(pin, active ? HIGH : LOW);
  }
}

void ignitionRelay(bool active) { setRelay(IGNITION_PIN, active); }
void killRelay(bool active) { setRelay(KILL_PIN, active); }
void starterRelay(bool active) { setRelay(START_PIN, active); }
void seatRelay(bool active) { setRelay(SEAT_PIN, active); }

void ignitionOnSequence() {
  killRelay(true);
  delay(IGNITION_SWITCH_DELAY);
  ignitionRelay(true);
  ignitionOn = true;
  Serial.println(">>> KILL KALDIRILDI");
  Serial.println(">>> KONTAK AÇ");
}

void ignitionOffSequence() {
  if (starterActive) {
    starterRelay(false);
    starterActive = false;
    lastStarterStopTime = millis();
    Serial.println(">>> MARŞ KONTAK KAPANDIĞI İÇİN KESİLDİ");
  }

  ignitionRelay(false);
  delay(IGNITION_SWITCH_DELAY);
  killRelay(false);
  ignitionOn = false;
  Serial.println(">>> KONTAK KAPAT");
  Serial.println(">>> KILL AKTİF");
}

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *server) override {
    deviceConnected = true;
    restartAdvertisingPending = false;
    Serial.println("📱 iPhone BLE bağlandı");
    lastStatusTime = millis();
  }

  void onDisconnect(BLEServer *server) override {
    deviceConnected = false;
    Serial.println("📱 iPhone BLE bağlantısı kesildi");
    restartAdvertisingPending = true;
    disconnectTime = millis();
  }
};

class CommandCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *characteristic) override {
    String command = characteristic->getValue().c_str();
    Serial.print("Komut geldi: ");
    Serial.println(command);

    if (command == "IGNITION_ON") {
      if (ignitionOn) {
        Serial.println(">>> KONTAK ZATEN AÇIK");
        return;
      }
      ignitionOnSequence();
      sendStatus();
    } else if (command == "IGNITION_OFF") {
      if (!ignitionOn) {
        Serial.println(">>> KONTAK ZATEN KAPALI");
        return;
      }
      ignitionOffSequence();
      sendStatus();
    } else if (command == "START") {
      if (!ignitionOn) {
        Serial.println(">>> MARŞ REDDEDİLDİ: kontak kapalı");
        return;
      }
      if (starterActive) {
        Serial.println(">>> MARŞ REDDEDİLDİ: zaten aktif");
        return;
      }
      if (lastStarterStopTime > 0 && millis() - lastStarterStopTime < STARTER_COOLDOWN) {
        Serial.println(">>> MARŞ REDDEDİLDİ: cooldown");
        return;
      }
      starterActive = true;
      starterStartTime = millis();
      starterRelay(true);
      Serial.println(">>> MARŞ BAŞLADI");
      sendStatus();
    } else if (command == "SEAT_OPEN") {
      if (seatActive) {
        Serial.println(">>> SELE REDDEDİLDİ: zaten aktif");
        return;
      }
      seatActive = true;
      seatStartTime = millis();
      seatRelay(true);
      Serial.println(">>> SELE AÇMA BAŞLADI");
      sendStatus();
    } else if (command == "STATUS") {
      Serial.println(">>> DURUM İSTENDİ");
      sendStatus();
    } else {
      Serial.print(">>> BİLİNMEYEN KOMUT: ");
      Serial.println(command);
    }
  }
};

void setup() {
  Serial.begin(115200);

  pinMode(IGNITION_PIN, OUTPUT);
  pinMode(KILL_PIN, OUTPUT);
  pinMode(START_PIN, OUTPUT);
  pinMode(SEAT_PIN, OUTPUT);

  ignitionRelay(false);
  killRelay(false);
  starterRelay(false);
  seatRelay(false);

  pinMode(BATTERY_PIN, INPUT);
  analogReadResolution(12);
  analogSetPinAttenuation(BATTERY_PIN, ADC_11db);

  BLEDevice::init("Revival50");
  BLEServer *server = BLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());

  BLEService *service = server->createService(SERVICE_UUID);
  commandCharacteristic = service->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_WRITE |
    BLECharacteristic::PROPERTY_NOTIFY
  );

  commandCharacteristic->setValue("Revival50 Ready");
  commandCharacteristic->setCallbacks(new CommandCallbacks());
  service->start();

  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(SERVICE_UUID);
  advertising->setScanResponse(true);
  BLEDevice::startAdvertising();

  Serial.println("BLE aktif: Revival50");
  Serial.println(RELAY_ACTIVE_LOW ? "Röle kontrolü: ACTIVE LOW" : "Röle kontrolü: ACTIVE HIGH / ULN2003");
  Serial.println("Başlangıç durumu: KONTAK OFF / KILL ACTIVE");

  lastStatusTime = millis();
}

float readBatteryVoltage() {
  uint32_t millivolts = analogReadMilliVolts(BATTERY_PIN);
  float adcVoltage = millivolts / 1000.0;
  return adcVoltage * ((R1 + R2) / R2);
}

void sendStatus() {
  if (commandCharacteristic == nullptr || !deviceConnected) return;

  float battery = readBatteryVoltage();
  String status = String("STATE:") + (ignitionOn ? "ON" : "OFF") +
                  ",STARTER:" + (starterActive ? "ON" : "OFF") +
                  ",SEAT:" + (seatActive ? "ON" : "OFF") +
                  ",BAT:" + String(battery, 2);

  commandCharacteristic->setValue(status.c_str());
  commandCharacteristic->notify();
  Serial.print("Durum gönderildi: ");
  Serial.println(status);
}

void loop() {
  unsigned long now = millis();

  if (starterActive && now - starterStartTime >= STARTER_DURATION) {
    starterRelay(false);
    starterActive = false;
    lastStarterStopTime = now;
    Serial.println(">>> MARŞ BİTTİ");
    sendStatus();
  }

  if (seatActive && now - seatStartTime >= SEAT_DURATION) {
    seatRelay(false);
    seatActive = false;
    Serial.println(">>> SELE AÇMA BİTTİ");
    sendStatus();
  }

  if (restartAdvertisingPending && now - disconnectTime >= ADVERTISING_RESTART_DELAY) {
    BLEDevice::startAdvertising();
    restartAdvertisingPending = false;
    Serial.println("📡 Revival50 tekrar advertising başladı");
  }

  if (deviceConnected && now - lastStatusTime >= STATUS_INTERVAL) {
    lastStatusTime = now;
    sendStatus();
  }

  delay(5);
}
