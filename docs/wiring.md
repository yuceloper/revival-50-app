# Revival 50 Wiring

## Kontak kabloları

Ölçülen fiziksel kontak davranışı:

- Kırmızı: sürekli +12V
- Siyah: kontak ON olduğunda +12V
- Gri: kill hattı
- Yeşil: GND

Kontak OFF:

- Gri <-> Yeşil bağlı
- Kırmızı ile Siyah ayrık

Kontak ON:

- Kırmızı <-> Siyah bağlı
- Gri ile Yeşil ayrık

## Röle görevleri

### Röle 1 - Kontak

- 30 -> Kırmızı
- 87 -> Siyah
- 87a -> boş

### Röle 2 - Kill

- 30 -> Gri
- 87a -> Yeşil
- 87 -> boş

Röle enerjisizken kill aktif olacak şekilde fail-safe çalışır.

### Röle 3 - Marş

- 30 -> fiziksel marş butonunun bir kumanda ucu
- 87 -> fiziksel marş butonunun diğer kumanda ucu
- 87a -> boş

Marş motorunun yüksek akımlı kalın kablosuna bağlanmaz.

### Röle 4 - Sele

- 30 -> sigortalı +12V
- 87 -> sele solenoidi +
- 87a -> boş
- solenoid - -> GND

## ULN2003A

ESP32 girişleri:

- GPIO4 -> IN1 -> Kontak
- GPIO7 -> IN2 -> Kill
- GPIO5 -> IN3 -> Marş
- GPIO6 -> IN4 -> Sele

ULN2003 beyaz 5-pin çıkış soketi, ölçülen düzene göre:

- Pin 1 -> Röle 1 pin 85
- Pin 2 -> Röle 2 pin 85
- Pin 3 -> Röle 3 pin 85
- Pin 4 -> Röle 4 pin 85
- Pin 5 -> ortak +12V -> tüm rölelerin pin 86 uçları

ULN2003:

- +5-12V -> sigortalı +12V
- GND -> ortak GND

ESP32 GND ile akü eksi ortak olmalıdır.

## Besleme

Akü +12V -> 3A sigorta -> buck IN+

Akü GND -> buck IN-

Buck OUT 5V -> ESP32 5V/VIN

Buck OUT GND -> ESP32 GND

Buck çıkışı ESP32'ye bağlanmadan önce multimetre ile 5V doğrulanmalıdır.

## Kontak sıralaması

IGNITION_ON:

1. Kill rölesi aktif -> gri/yeşil ayrılır
2. 75ms bekle
3. Kontak rölesi aktif -> kırmızı/siyah birleşir

IGNITION_OFF:

1. Marş aktifse kes
2. Kontak rölesini bırak -> kırmızı/siyah ayrılır
3. 75ms bekle
4. Kill rölesini bırak -> gri/yeşil birleşir

## Güvenlik

- Montaj sırasında akünün eksi kutbunu sökün.
- ESP32 GPIO pinlerine veya 5V girişine 12V vermeyin.
- Gri kill hattına +12V vermeyin.
- Motor tesisatına bağlamadan önce dört rölenin bobin tarafını masa üzerinde test edin.
