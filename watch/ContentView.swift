import SwiftUI

struct ContentView: View {

    @StateObject private var watchSession = WatchSessionManager.shared

    var body: some View {
        ScrollView {
            VStack(spacing: 10) {
                Image(systemName: "scooter")
                    .font(.system(size: 30))

                Text("Revival 50")
                    .font(.headline)

                HStack {
                    Image(systemName: "battery.75percent")
                    if watchSession.batteryVoltage > 0 {
                        Text(String(format: "%.2f V", watchSession.batteryVoltage))
                    } else {
                        Text("--.-- V")
                    }
                }
                .font(.caption)

                HStack {
                    Circle()
                        .frame(width: 7, height: 7)
                        .foregroundStyle(watchSession.ignitionOn ? .green : .secondary)

                    Text(watchSession.ignitionOn ? "Kontak Açık" : "Kontak Kapalı")
                }
                .font(.caption)

                Text(watchSession.isReachable ? "iPhone hazır" : "iPhone beklemede")
                    .font(.caption2)
                    .foregroundStyle(watchSession.isReachable ? .green : .secondary)

                if !watchSession.lastResult.isEmpty {
                    Text(watchSession.lastResult)
                        .font(.caption2)
                        .multilineTextAlignment(.center)
                }

                Button {
                    watchSession.sendCommand(
                        watchSession.ignitionOn ? "IGNITION_OFF" : "IGNITION_ON"
                    )
                } label: {
                    Label(
                        watchSession.ignitionOn ? "Kapat" : "Kontak",
                        systemImage: "power"
                    )
                }
                .buttonStyle(.borderedProminent)

                Button {
                    watchSession.sendCommand("START")
                } label: {
                    Label(
                        watchSession.starterActive ? "Marş..." : "Marş",
                        systemImage: watchSession.starterActive ? "bolt.circle.fill" : "bolt.fill"
                    )
                }
                .disabled(!watchSession.ignitionOn || watchSession.starterActive)

                Button {
                    watchSession.sendCommand("SEAT_OPEN")
                } label: {
                    Label(
                        watchSession.seatActive ? "Açılıyor..." : "Sele",
                        systemImage: "lock.open.fill"
                    )
                }
                .disabled(watchSession.seatActive)
            }
            .padding()
        }
    }
}

#Preview {
    ContentView()
}
