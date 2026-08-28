import Foundation
import WatchConnectivity
import Combine

final class WatchSessionManager: NSObject, ObservableObject, WCSessionDelegate {

    static let shared = WatchSessionManager()

    var onCommandReceived: ((String) -> Bool)?

    private override init() {
        super.init()

        if WCSession.isSupported() {
            WCSession.default.delegate = self
            WCSession.default.activate()
        }
    }

    func session(
        _ session: WCSession,
        activationDidCompleteWith activationState: WCSessionActivationState,
        error: Error?
    ) {
        print("iPhone WatchSession aktif: \(activationState.rawValue)")
    }

    func sessionDidBecomeInactive(_ session: WCSession) {
        print("WatchSession inactive")
    }

    func sessionDidDeactivate(_ session: WCSession) {
        print("WatchSession deactivated")
        session.activate()
    }

    func session(
        _ session: WCSession,
        didReceiveMessage message: [String : Any]
    ) {
        guard let command = message["command"] as? String else {
            return
        }

        print("⌚ Watch komutu geldi: \(command)")

        DispatchQueue.main.async {
            self.onCommandReceived?(command)
        }
    }
    
    func session(
        _ session: WCSession,
        didReceiveMessage message: [String : Any],
        replyHandler: @escaping ([String : Any]) -> Void
    ) {
        guard let command = message["command"] as? String else {
            replyHandler([
                "success": false,
                "error": "Komut bulunamadı"
            ])
            return
        }

        print("⌚ Watch komutu geldi: \(command)")

        DispatchQueue.main.async {

            let success = self.onCommandReceived?(command) ?? false

            if success {
                replyHandler([
                    "success": true,
                    "command": command
                ])
            } else {
                replyHandler([
                    "success": false,
                    "command": command,
                    "error": "ESP32 bağlı değil"
                ])
            }
        }
    }
    
    func sendStatusToWatch(
        ignitionOn: Bool,
        starterActive: Bool,
        seatActive: Bool,
        batteryVoltage: Double
    ) {
        let session = WCSession.default

        guard session.activationState == .activated else {
            print("⌚ Status gönderilemedi: WCSession aktif değil")
            return
        }

        let status: [String: Any] = [
            "type": "status",
            "ignitionOn": ignitionOn,
            "starterActive": starterActive,
            "seatActive": seatActive,
            "batteryVoltage": batteryVoltage
        ]

        // 1) Kalıcı son durum
        // Watch daha sonra açılırsa receivedApplicationContext'ten okuyabilir.
        do {
            try session.updateApplicationContext(status)

            print(
                "⌚ Watch context → " +
                "ignition=\(ignitionOn), " +
                "starter=\(starterActive), " +
                "seat=\(seatActive), " +
                "battery=\(batteryVoltage)"
            )
        } catch {
            print(
                "❌ Watch context hatası: \(error.localizedDescription)"
            )
        }

        // 2) Watch şu anda erişilebilirse ANLIK gönder
        if session.isReachable {

            session.sendMessage(
                status,
                replyHandler: nil,
                errorHandler: { error in
                    print(
                        "❌ Watch canlı status hatası: \(error.localizedDescription)"
                    )
                }
            )

            print("⌚ Watch canlı status gönderildi")
        }
    }
}
