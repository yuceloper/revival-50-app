# Revival 50 App

Apple Watch / iPhone kontrollü Mondial Revival 50 retrofit projesi.

## Mimari

- Apple Watch -> WatchConnectivity -> iPhone
- iPhone -> CoreBluetooth BLE -> ESP32-S3
- ESP32-S3 -> ULN2003A -> 4x 12V otomotiv rölesi
- Röleler: kontak, kill, marş, sele

## Proje yapısı

- `RevivalControl.xcodeproj/` gerçek Xcode projesi
- `RevivalControl/` iPhone uygulamasının gerçek kaynakları
- `RevivalWatch Watch App/` Apple Watch uygulamasının gerçek kaynakları
- `RevivalWatch-Watch-App-Info.plist` Watch target yapılandırması
- `firmware/revival50/revival50.ino` ESP32-S3 firmware
- `docs/wiring.md` bağlantı ve montaj dokümantasyonu

> Not: `ios/`, `watch/` ve kökteki `revival50.ino` ilk repo kurulumunda oluşturulan snapshot/duplicate kopyalardır. Gerçek geliştirme kaynakları yukarıdaki Xcode klasörleri ve `firmware/` altındadır. Bu kopyalar sonraki temizlikte kaldırılmalıdır.

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

## Donanım güvenliği

- Motor tesisatı üzerinde çalışırken akünün eksi kutbunu sökün.
- ESP32 GPIO pinlerine doğrudan 12V bağlamayın.
- Marş rölesi marş motorunun yüksek akımlı kalın kablosuna değil, marş butonunun kumanda hattına paralel bağlanır.
- Gri kill hattına +12V uygulanmaz.
- Buck converter çıkışı ESP32'ye bağlanmadan önce multimetre ile 5V doğrulanmalıdır.

## Geliştirme notu

Yeni iPhone değişiklikleri `RevivalControl/` altında, Watch değişiklikleri `RevivalWatch Watch App/` altında yapılmalıdır. `ios/` ve `watch/` dizinleri kaynak olarak kullanılmamalıdır.
