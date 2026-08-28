import Foundation
import WatchConnectivity
import Combine

final class WatchSessionManager: NSObject, ObservableObject, WCSessionDelegate {

    static let shared = WatchSessionManager()

    @Published var isReachable = false
    @Published var activationText = "Hazırlanıyor..."
    @Published var lastResult = ""
    @Published var ignitionOn = false
    @Published var starterActive = false
    @Published var seatActive = false
    @Published var batteryVoltage: Double = 0

    private override init() {
        super.init()

        guard WCSession.isSupported() else {
            activationText = "WatchConnectivity desteklenmiyor"
            return
        }

        let session = WCSession.default
        session.delegate = self
        session.activate()
    }

    func session(
        _ session: WCSession,
        activationDidCompleteWith activationState: WCSessionActivationState,
        error: Error?
    ) {
        if let error {
            DispatchQueue.main.async {
                self.activationText = "Activation hata"
            }
            print("❌ WCSession activation hata: \(error.localizedDescription)")
            return
        }

        DispatchQueue.main.async {
            switch activationState {
            case .activated:
                self.activationText = "Session aktif"
            case .inactive:
                self.activationText = "Session inactive"
            case .notActivated:
                self.activationText = "Session aktif değil"
            @unknown default:
                self.activationText = "Bilinmeyen durum"
            }

            self.isReachable = session.isReachable
        }

        let context = session.receivedApplicationContext
        if !context.isEmpty {
            applyStatus(context)
        }
    }

    func sessionReachabilityDidChange(_ session: WCSession) {
        DispatchQueue.main.async {
            self.isReachable = session.isReachable
        }
    }

    func sendCommand(_ command: String) {
        let session = WCSession.default

        guard session.activationState == .activated else {
            DispatchQueue.main.async {
                self.lastResult = "Session aktif değil ❌"
            }
            return
        }

        session.sendMessage(
            ["command": command],
            replyHandler: { reply in
                DispatchQueue.main.async {
                    let success = reply["success"] as? Bool ?? false

                    if success {
                        switch command {
                        case "IGNITION_ON":
                            self.lastResult = "Kontak açma komutu ✅"
                        case "IGNITION_OFF":
                            self.lastResult = "Kontak kapatma komutu ✅"
                        case "START":
                            self.lastResult = "Marş komutu ✅"
                        case "SEAT_OPEN":
                            self.lastResult = "Sele komutu ✅"
                        default:
                            self.lastResult = "Komut başarılı ✅"
                        }
                    } else {
                        let error = reply["error"] as? String ?? "Bilinmeyen hata"
                        self.lastResult = "\(error) ❌"
                    }
                }
            },
            errorHandler: { error in
                DispatchQueue.main.async {
                    self.lastResult = "Gönderilemedi ❌"
                }
                print("Watch mesaj hatası: \(error.localizedDescription)")
            }
        )
    }

    private func applyStatus(_ data: [String: Any]) {
        DispatchQueue.main.async {
            if let ignition = data["ignitionOn"] as? Bool {
                self.ignitionOn = ignition
            }
            if let starter = data["starterActive"] as? Bool {
                self.starterActive = starter
            }
            if let seat = data["seatActive"] as? Bool {
                self.seatActive = seat
            }
            if let voltage = data["batteryVoltage"] as? Double {
                self.batteryVoltage = voltage
            }
        }
    }

    func session(
        _ session: WCSession,
        didReceiveApplicationContext applicationContext: [String: Any]
    ) {
        applyStatus(applicationContext)
    }

    func session(
        _ session: WCSession,
        didReceiveMessage message: [String: Any]
    ) {
        guard let type = message["type"] as? String, type == "status" else {
            return
        }
        applyStatus(message)
    }
}
