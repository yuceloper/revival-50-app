# Revival 50 App

Apple Watch / iPhone kontrollü Mondial Revival 50 retrofit projesi.

## Mimari

- Apple Watch -> WatchConnectivity -> iPhone
- iPhone -> CoreBluetooth BLE -> ESP32-S3
- ESP32-S3 -> ULN2003A -> 4x 12V otomotiv rölesi
- Röleler: kontak, kill, marş, sele

## Dizinler

- `firmware/` ESP32-S3 Arduino firmware
- `ios/` iPhone SwiftUI / CoreBluetooth kaynakları
- `watch/` Apple Watch SwiftUI / WatchConnectivity kaynakları
- `docs/` bağlantı ve montaj notları

## BLE

Device name: `Revival50`

Service UUID: `12345678-1234-1234-1234-1234567890ab`

Characteristic UUID: `abcdefab-1234-5678-1234-abcdefabcdef`

Komutlar:

- `IGNITION_ON`
- `IGNITION_OFF`
- `START`
- `SEAT_OPEN`
- `STATUS`

ESP32 status örneği:

`STATE:ON,STARTER:OFF,SEAT:OFF,BAT:12.64`

## ESP32 pinleri

- GPIO4: Ignition
- GPIO5: Starter
- GPIO6: Seat
- GPIO7: Kill
- GPIO1: Battery ADC

## Güvenlik

Motor tesisatı üzerinde çalışırken akünün eksi kutbunu sökün. ESP32 GPIO pinlerine doğrudan 12V bağlamayın. Marş rölesi marş motorunun kalın güç kablosuna değil, marş butonunun kumanda hattına paralel bağlanır.
