#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

// ==================================================
// BLE
// ==================================================

#define SERVICE_UUID        "12345678-1234-1234-1234-1234567890ab"
#define CHARACTERISTIC_UUID "abcdefab-1234-5678-1234-abcdefabcdef"

// ==================================================
// GPIO
// ==================================================

#define IGNITION_PIN 4
#define START_PIN    5
#define SEAT_PIN     6
#define KILL_PIN     7

#define BATTERY_PIN  1

// ==================================================
// RÖLE POLARİTESİ
// ==================================================
//
// ULN2003A:
// ESP32 HIGH -> ULN2003 çıkışı GND'ye çeker
//             -> 12V otomotiv rölesi AKTİF
//
// Bu yüzden ACTIVE HIGH.
//

const bool RELAY_ACTIVE_LOW = false;

// ==================================================
// AKÜ VOLTAJ BÖLÜCÜ
// ==================================================

#define R1 100000.0
#define R2 27000.0

// ==================================================
// GENEL DURUM
// ==================================================

bool ignitionOn = false;
bool deviceConnected = false;

// ==================================================
// MARŞ
// ==================================================

bool starterActive = false;

unsigned long starterStartTime = 0;
unsigned long lastStarterStopTime = 0;

const unsigned long STARTER_DURATION = 1000;
const unsigned long STARTER_COOLDOWN = 1500;

// ==================================================
// SELE
// ==================================================

bool seatActive = false;

unsigned long seatStartTime = 0;

const unsigned long SEAT_DURATION = 500;

// ==================================================
// KONTAK / KILL GEÇİŞ SÜRESİ
// ==================================================

const unsigned long IGNITION_SWITCH_DELAY = 75;

// ==================================================
// BLE RECONNECT
// ==================================================

bool restartAdvertisingPending = false;

unsigned long disconnectTime = 0;

const unsigned long ADVERTISING_RESTART_DELAY = 300;

// ==================================================
// PERİYODİK STATUS
// ==================================================

unsigned long lastStatusTime = 0;

const unsigned long STATUS_INTERVAL = 2000;

// ==================================================
// BLE
// ==================================================

BLECharacteristic *commandCharacteristic = nullptr;

// ==================================================
// FUNCTION PROTOTYPES
// ==================================================

float readBatteryVoltage();
void sendStatus();

void setRelay(uint8_t pin, bool active);

void ignitionRelay(bool active);
void killRelay(bool active);
void starterRelay(bool active);
void seatRelay(bool active);

void ignitionOnSequence();
void ignitionOffSequence();

// ==================================================
// RÖLE KONTROLÜ
// ==================================================

void setRelay(uint8_t pin, bool active) {

  if (RELAY_ACTIVE_LOW) {

    digitalWrite(
      pin,
      active ? LOW : HIGH
    );

  } else {

    digitalWrite(
      pin,
      active ? HIGH : LOW
    );
  }
}

void ignitionRelay(bool active) {
  setRelay(IGNITION_PIN, active);
}

void killRelay(bool active) {
  setRelay(KILL_PIN, active);
}

void starterRelay(bool active) {
  setRelay(START_PIN, active);
}

void seatRelay(bool active) {
  setRelay(SEAT_PIN, active);
}

// ==================================================
// KONTAK AÇMA SIRASI
// ==================================================
//
// Fiziksel kontak:
//
// OFF:
// GRİ <-> YEŞİL bağlı
// kill aktif
//
// ON:
// KIRMIZI <-> SİYAH bağlı
// kill açık
//
// IGNITION_ON:
// 1) önce kill rölesini çek
//    -> gri/yeşil ayrılır
// 2) sonra kontak rölesini çek
//    -> kırmızı/siyah birleşir
//

void ignitionOnSequence() {

  // Önce KILL'i kaldır
  killRelay(true);

  delay(IGNITION_SWITCH_DELAY);

  // Sonra kontak beslemesini ver
  ignitionRelay(true);

  ignitionOn = true;

  Serial.println(">>> KILL KALDIRILDI");
  Serial.println(">>> KONTAK AÇ");
}

// ==================================================
// KONTAK KAPATMA SIRASI
// ==================================================
//
// IGNITION_OFF:
// 1) varsa marşı kes
// 2) kırmızı/siyahı ayır
// 3) sonra kill rölesini bırak
//    -> gri/yeşil tekrar birleşir
//

void ignitionOffSequence() {

  // Marş açıksa hemen kes
  if (starterActive) {

    starterRelay(false);

    starterActive = false;

    lastStarterStopTime = millis();

    Serial.println(
      ">>> MARŞ KONTAK KAPANDIĞI İÇİN KESİLDİ"
    );
  }

  // Önce kontak beslemesini kes
  ignitionRelay(false);

  delay(IGNITION_SWITCH_DELAY);

  // Sonra kill'i aktif et
  killRelay(false);

  ignitionOn = false;

  Serial.println(">>> KONTAK KAPAT");
  Serial.println(">>> KILL AKTİF");
}

// ==================================================
// BLE SERVER CALLBACK
// ==================================================

class ServerCallbacks : public BLEServerCallbacks {

  void onConnect(BLEServer *server) override {

    deviceConnected = true;

    restartAdvertisingPending = false;

    Serial.println("📱 iPhone BLE bağlandı");

    lastStatusTime = millis();
  }

  void onDisconnect(BLEServer *server) override {

    deviceConnected = false;

    Serial.println(
      "📱 iPhone BLE bağlantısı kesildi"
    );

    restartAdvertisingPending = true;
    disconnectTime = millis();
  }
};

// ==================================================
// BLE COMMAND CALLBACK
// ==================================================

class CommandCallbacks : public BLECharacteristicCallbacks {

  void onWrite(
    BLECharacteristic *characteristic
  ) override {

    String command =
      characteristic->getValue().c_str();

    Serial.print("Komut geldi: ");
    Serial.println(command);

    // --------------------------------------------------
    // KONTAK AÇ
    // --------------------------------------------------

    if (command == "IGNITION_ON") {

      if (ignitionOn) {

        Serial.println(
          ">>> KONTAK ZATEN AÇIK"
        );

        return;
      }

      ignitionOnSequence();

      sendStatus();
    }

    // --------------------------------------------------
    // KONTAK KAPAT
    // --------------------------------------------------

    else if (command == "IGNITION_OFF") {

      if (!ignitionOn) {

        Serial.println(
          ">>> KONTAK ZATEN KAPALI"
        );

        return;
      }

      ignitionOffSequence();

      sendStatus();
    }

    // --------------------------------------------------
    // MARŞ
    // --------------------------------------------------

    else if (command == "START") {

      // Kontak açık değilse marş yok
      if (!ignitionOn) {

        Serial.println(
          ">>> MARŞ REDDEDİLDİ: kontak kapalı"
        );

        return;
      }

      // Marş zaten çalışıyorsa tekrar tetikleme
      if (starterActive) {

        Serial.println(
          ">>> MARŞ REDDEDİLDİ: zaten aktif"
        );

        return;
      }

      // Cooldown
      if (
        lastStarterStopTime > 0 &&
        millis() - lastStarterStopTime <
        STARTER_COOLDOWN
      ) {

        Serial.println(
          ">>> MARŞ REDDEDİLDİ: cooldown"
        );

        return;
      }

      starterActive = true;

      starterStartTime = millis();

      starterRelay(true);

      Serial.println(
        ">>> MARŞ BAŞLADI"
      );

      sendStatus();
    }

    // --------------------------------------------------
    // SELE
    // --------------------------------------------------

    else if (command == "SEAT_OPEN") {

      if (seatActive) {

        Serial.println(
          ">>> SELE REDDEDİLDİ: zaten aktif"
        );

        return;
      }

      seatActive = true;

      seatStartTime = millis();

      seatRelay(true);

      Serial.println(
        ">>> SELE AÇMA BAŞLADI"
      );

      sendStatus();
    }

    // --------------------------------------------------
    // STATUS
    // --------------------------------------------------

    else if (command == "STATUS") {

      Serial.println(
        ">>> DURUM İSTENDİ"
      );

      sendStatus();
    }

    // --------------------------------------------------
    // BİLİNMEYEN KOMUT
    // --------------------------------------------------

    else {

      Serial.print(
        ">>> BİLİNMEYEN KOMUT: "
      );

      Serial.println(command);
    }
  }
};

// ==================================================
// SETUP
// ==================================================

void setup() {

  Serial.begin(115200);

  // --------------------------------------------------
  // GPIO
  // --------------------------------------------------

  pinMode(
    IGNITION_PIN,
    OUTPUT
  );

  pinMode(
    KILL_PIN,
    OUTPUT
  );

  pinMode(
    START_PIN,
    OUTPUT
  );

  pinMode(
    SEAT_PIN,
    OUTPUT
  );

  // --------------------------------------------------
  // GÜVENLİ BAŞLANGIÇ
  // --------------------------------------------------
  //
  // Kontak rölesi: pasif
  // Kill rölesi: pasif
  //
  // Kill rölesi pasif olduğunda:
  // 30 <-> 87a
  // GRİ <-> YEŞİL
  // yani motor güvenli şekilde kill durumda.
  //

  ignitionRelay(false);

  killRelay(false);

  starterRelay(false);

  seatRelay(false);

  ignitionOn = false;
  starterActive = false;
  seatActive = false;

  // --------------------------------------------------
  // ADC
  // --------------------------------------------------

  pinMode(
    BATTERY_PIN,
    INPUT
  );

  analogReadResolution(12);

  analogSetPinAttenuation(
    BATTERY_PIN,
    ADC_11db
  );

  // --------------------------------------------------
  // BLE
  // --------------------------------------------------

  BLEDevice::init(
    "Revival50"
  );

  BLEServer *server =
    BLEDevice::createServer();

  server->setCallbacks(
    new ServerCallbacks()
  );

  BLEService *service =
    server->createService(
      SERVICE_UUID
    );

  commandCharacteristic =
    service->createCharacteristic(
      CHARACTERISTIC_UUID,

      BLECharacteristic::PROPERTY_READ |
      BLECharacteristic::PROPERTY_WRITE |
      BLECharacteristic::PROPERTY_NOTIFY
    );

  commandCharacteristic->setValue(
    "Revival50 Ready"
  );

  commandCharacteristic->setCallbacks(
    new CommandCallbacks()
  );

  service->start();

  // --------------------------------------------------
  // ADVERTISING
  // --------------------------------------------------

  BLEAdvertising *advertising =
    BLEDevice::getAdvertising();

  advertising->addServiceUUID(
    SERVICE_UUID
  );

  advertising->setScanResponse(
    true
  );

  BLEDevice::startAdvertising();

  Serial.println(
    "BLE aktif: Revival50"
  );

  Serial.println(
    RELAY_ACTIVE_LOW
      ? "Röle kontrolü: ACTIVE LOW"
      : "Röle kontrolü: ACTIVE HIGH / ULN2003"
  );

  Serial.println(
    "Başlangıç durumu: KONTAK OFF / KILL ACTIVE"
  );

  lastStatusTime = millis();
}

// ==================================================
// AKÜ VOLTAJI
// ==================================================

float readBatteryVoltage() {

  uint32_t millivolts =
    analogReadMilliVolts(
      BATTERY_PIN
    );

  float adcVoltage =
    millivolts / 1000.0;

  float batteryVoltage =
    adcVoltage *
    (
      (R1 + R2)
      /
      R2
    );

  return batteryVoltage;
}

// ==================================================
// STATUS GÖNDER
// ==================================================

void sendStatus() {

  if (
    commandCharacteristic == nullptr ||
    !deviceConnected
  ) {

    return;
  }

  float battery =
    readBatteryVoltage();

  String status =
    String("STATE:")
    +
    (
      ignitionOn
        ? "ON"
        : "OFF"
    )
    +
    ",STARTER:"
    +
    (
      starterActive
        ? "ON"
        : "OFF"
    )
    +
    ",SEAT:"
    +
    (
      seatActive
        ? "ON"
        : "OFF"
    )
    +
    ",BAT:"
    +
    String(
      battery,
      2
    );

  commandCharacteristic->setValue(
    status.c_str()
  );

  commandCharacteristic->notify();

  Serial.print(
    "Durum gönderildi: "
  );

  Serial.println(status);
}

// ==================================================
// LOOP
// ==================================================

void loop() {

  unsigned long now =
    millis();

  // --------------------------------------------------
  // MARŞ SÜRESİ
  // --------------------------------------------------

  if (
    starterActive &&
    now - starterStartTime >=
    STARTER_DURATION
  ) {

    starterRelay(false);

    starterActive = false;

    lastStarterStopTime = now;

    Serial.println(
      ">>> MARŞ BİTTİ"
    );

    sendStatus();
  }

  // --------------------------------------------------
  // SELE SÜRESİ
  // --------------------------------------------------

  if (
    seatActive &&
    now - seatStartTime >=
    SEAT_DURATION
  ) {

    seatRelay(false);

    seatActive = false;

    Serial.println(
      ">>> SELE AÇMA BİTTİ"
    );

    sendStatus();
  }

  // --------------------------------------------------
  // ADVERTISING RESTART
  // --------------------------------------------------

  if (
    restartAdvertisingPending &&
    now - disconnectTime >=
    ADVERTISING_RESTART_DELAY
  ) {

    BLEDevice::startAdvertising();

    restartAdvertisingPending = false;

    Serial.println(
      "📡 Revival50 tekrar advertising başladı"
    );
  }

  // --------------------------------------------------
  // PERİYODİK STATUS
  // --------------------------------------------------

  if (
    deviceConnected &&
    now - lastStatusTime >=
    STATUS_INTERVAL
  ) {

    lastStatusTime = now;

    sendStatus();
  }

  delay(5);
}