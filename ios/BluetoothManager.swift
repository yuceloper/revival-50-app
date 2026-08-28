import Foundation
import CoreBluetooth
import Combine

final class BluetoothManager:
    NSObject,
    ObservableObject,
    CBCentralManagerDelegate,
    CBPeripheralDelegate {

    @Published var statusText: String = "Bluetooth hazırlanıyor..."
    @Published var foundRevival = false
    @Published var connected = false
    @Published var ignitionOn = false
    @Published var batteryVoltage: Double = 0
    @Published var starterActive = false
    @Published var seatActive = false

    private var revivalPeripheral: CBPeripheral?

    private let serviceUUID = CBUUID(
        string: "12345678-1234-1234-1234-1234567890ab"
    )

    private let characteristicUUID = CBUUID(
        string: "abcdefab-1234-5678-1234-abcdefabcdef"
    )

    private var commandCharacteristic: CBCharacteristic?
    private var centralManager: CBCentralManager!

    override init() {
        super.init()

        centralManager = CBCentralManager(
            delegate: self,
            queue: nil
        )

        WatchSessionManager.shared.onCommandReceived = { [weak self] command in
            self?.sendCommand(command) ?? false
        }
    }

    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        switch central.state {
        case .poweredOn:
            startScanning()
        case .poweredOff:
            statusText = "Bluetooth kapalı"
        case .unauthorized:
            statusText = "Bluetooth izni verilmemiş"
        case .unsupported:
            statusText = "Bluetooth desteklenmiyor"
        case .resetting:
            statusText = "Bluetooth yeniden başlatılıyor..."
        case .unknown:
            statusText = "Bluetooth durumu bilinmiyor"
        @unknown default:
            statusText = "Bilinmeyen Bluetooth durumu"
        }
    }

    func centralManager(
        _ central: CBCentralManager,
        didDiscover peripheral: CBPeripheral,
        advertisementData: [String: Any],
        rssi RSSI: NSNumber
    ) {
        let name = peripheral.name
            ?? advertisementData[CBAdvertisementDataLocalNameKey] as? String
            ?? "İsimsiz cihaz"

        print("BLE bulundu: \(name) RSSI: \(RSSI)")

        if name == "Revival50" {
            foundRevival = true
            statusText = "Revival50 bulundu, bağlanılıyor..."
            revivalPeripheral = peripheral
            central.stopScan()
            central.connect(peripheral, options: nil)
        }
    }

    func centralManager(
        _ central: CBCentralManager,
        didConnect peripheral: CBPeripheral
    ) {
        connected = true
        statusText = "Revival50 bağlandı ✅"
        peripheral.delegate = self
        peripheral.discoverServices([serviceUUID])
        print("ESP32 bağlantısı başarılı")
    }

    func centralManager(
        _ central: CBCentralManager,
        didDisconnectPeripheral peripheral: CBPeripheral,
        error: Error?
    ) {
        connected = false
        foundRevival = false
        commandCharacteristic = nil

        ignitionOn = false
        starterActive = false
        seatActive = false
        batteryVoltage = 0

        if let error {
            print("ESP32 bağlantısı kesildi: \(error.localizedDescription)")
        } else {
            print("ESP32 bağlantısı kesildi")
        }

        statusText = "Bağlantı kesildi, tekrar aranıyor..."

        WatchSessionManager.shared.sendStatusToWatch(
            ignitionOn: false,
            starterActive: false,
            seatActive: false,
            batteryVoltage: 0
        )

        DispatchQueue.main.asyncAfter(deadline: .now() + 1.0) {
            self.startScanning()
        }
    }

    func centralManager(
        _ central: CBCentralManager,
        didFailToConnect peripheral: CBPeripheral,
        error: Error?
    ) {
        connected = false
        foundRevival = false
        commandCharacteristic = nil
        print("Bağlantı başarısız: \(error?.localizedDescription ?? "Bilinmeyen hata")")
        statusText = "Bağlanamadı, tekrar deneniyor..."

        DispatchQueue.main.asyncAfter(deadline: .now() + 1.0) {
            self.startScanning()
        }
    }

    func peripheral(
        _ peripheral: CBPeripheral,
        didDiscoverServices error: Error?
    ) {
        guard error == nil else {
            print("Service discovery hatası: \(error!)")
            return
        }
        guard let services = peripheral.services else { return }

        for service in services where service.uuid == serviceUUID {
            peripheral.discoverCharacteristics([characteristicUUID], for: service)
        }
    }

    func peripheral(
        _ peripheral: CBPeripheral,
        didDiscoverCharacteristicsFor service: CBService,
        error: Error?
    ) {
        guard error == nil else {
            print("Characteristic discovery hatası: \(error!)")
            return
        }
        guard let characteristics = service.characteristics else { return }

        for characteristic in characteristics where characteristic.uuid == characteristicUUID {
            commandCharacteristic = characteristic
            peripheral.setNotifyValue(true, for: characteristic)
            statusText = "Revival50 hazır ✅"
            print("Komut characteristic bulundu")
        }
    }

    @discardableResult
    func sendCommand(_ command: String) -> Bool {
        guard let peripheral = revivalPeripheral,
              peripheral.state == .connected,
              let characteristic = commandCharacteristic,
              let data = command.data(using: .utf8) else {
            print("Komut gönderilemedi: BLE hazır değil")
            return false
        }

        peripheral.writeValue(data, for: characteristic, type: .withResponse)
        print("Gönderildi: \(command)")
        return true
    }

    private func startScanning() {
        guard centralManager.state == .poweredOn else { return }

        if centralManager.isScanning {
            centralManager.stopScan()
        }

        foundRevival = false
        statusText = "Revival50 aranıyor..."

        centralManager.scanForPeripherals(
            withServices: [serviceUUID],
            options: [CBCentralManagerScanOptionAllowDuplicatesKey: false]
        )

        print("BLE taraması başladı")
    }

    func peripheral(
        _ peripheral: CBPeripheral,
        didUpdateValueFor characteristic: CBCharacteristic,
        error: Error?
    ) {
        if let error {
            print("BLE notification hatası: \(error.localizedDescription)")
            return
        }

        guard characteristic.uuid == characteristicUUID,
              let data = characteristic.value,
              let value = String(data: data, encoding: .utf8),
              value.hasPrefix("STATE:") else {
            return
        }

        print("ESP32 durum: \(value)")

        let parts = value.split(separator: ",")
        var newIgnition = ignitionOn
        var newStarter = starterActive
        var newSeat = seatActive
        var newBattery = batteryVoltage

        for partSubstring in parts {
            let part = String(partSubstring)

            if part.hasPrefix("STATE:") {
                newIgnition = part.replacingOccurrences(of: "STATE:", with: "") == "ON"
            } else if part.hasPrefix("STARTER:") {
                newStarter = part.replacingOccurrences(of: "STARTER:", with: "") == "ON"
            } else if part.hasPrefix("SEAT:") {
                newSeat = part.replacingOccurrences(of: "SEAT:", with: "") == "ON"
            } else if part.hasPrefix("BAT:") {
                let voltageString = part.replacingOccurrences(of: "BAT:", with: "")
                if let voltage = Double(voltageString) {
                    newBattery = voltage
                }
            }
        }

        DispatchQueue.main.async {
            self.ignitionOn = newIgnition
            self.starterActive = newStarter
            self.seatActive = newSeat
            self.batteryVoltage = newBattery

            WatchSessionManager.shared.sendStatusToWatch(
                ignitionOn: newIgnition,
                starterActive: newStarter,
                seatActive: newSeat,
                batteryVoltage: newBattery
            )
        }
    }

    func peripheral(
        _ peripheral: CBPeripheral,
        didUpdateNotificationStateFor characteristic: CBCharacteristic,
        error: Error?
    ) {
        if let error {
            print("Notify açma hatası: \(error.localizedDescription)")
            return
        }

        guard characteristic.uuid == characteristicUUID,
              characteristic.isNotifying else {
            return
        }

        print("BLE notifications aktif ✅")
        sendCommand("STATUS")
    }
}
