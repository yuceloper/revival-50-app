import SwiftUI

struct ContentView: View {

    @StateObject private var bluetooth = BluetoothManager()

    var body: some View {

        VStack(spacing: 22) {

            Image(systemName: "scooter")
                .font(.system(size: 70))
                .padding(.bottom, 5)

            Text("Revival 50")
                .font(.largeTitle)
                .bold()

            // MARK: BLE STATUS

            Text(bluetooth.statusText)
                .font(.subheadline)
                .foregroundStyle(
                    bluetooth.connected
                        ? .green
                        : .secondary
                )

            // MARK: MOTOR STATUS

            VStack(spacing: 6) {

                HStack {

                    Circle()
                        .frame(width: 10, height: 10)
                        .foregroundStyle(
                            bluetooth.ignitionOn
                                ? .green
                                : .secondary
                        )

                    Text(
                        bluetooth.ignitionOn
                            ? "Kontak Açık"
                            : "Kontak Kapalı"
                    )
                    .font(.headline)
                }

                HStack {
                    Image(systemName: "battery.75percent")

                    if bluetooth.batteryVoltage > 0 {
                        Text(String(format: "%.2f V", bluetooth.batteryVoltage))
                    } else {
                        Text("--.-- V")
                    }
                }
                .font(.headline)
                .foregroundStyle(.secondary)
            }

            // MARK: IGNITION

            Button {
                bluetooth.sendCommand(
                    bluetooth.ignitionOn
                        ? "IGNITION_OFF"
                        : "IGNITION_ON"
                )
            } label: {
                Label(
                    bluetooth.ignitionOn
                        ? "Kontağı Kapat"
                        : "Kontağı Aç",
                    systemImage:
                        bluetooth.ignitionOn
                        ? "power.circle.fill"
                        : "power.circle"
                )
                .font(.title3)
                .frame(maxWidth: .infinity)
                .padding()
            }
            .buttonStyle(.borderedProminent)
            .disabled(!bluetooth.connected)

            // MARK: START

            Button {
                bluetooth.sendCommand("START")
            } label: {
                Label(
                    bluetooth.starterActive
                        ? "Marş Basılıyor..."
                        : "Marş Bas",
                    systemImage:
                        bluetooth.starterActive
                        ? "bolt.circle.fill"
                        : "bolt.fill"
                )
                .font(.title3)
                .frame(maxWidth: .infinity)
                .padding()
            }
            .buttonStyle(.bordered)
            .disabled(
                !bluetooth.connected ||
                !bluetooth.ignitionOn ||
                bluetooth.starterActive
            )

            // MARK: SEAT

            Button {
                bluetooth.sendCommand("SEAT_OPEN")
            } label: {
                Label(
                    bluetooth.seatActive
                        ? "Sele Açılıyor..."
                        : "Seleyi Aç",
                    systemImage:
                        bluetooth.seatActive
                        ? "lock.open.fill"
                        : "lock.open"
                )
                .font(.title3)
                .frame(maxWidth: .infinity)
                .padding()
            }
            .buttonStyle(.bordered)
            .disabled(
                !bluetooth.connected ||
                bluetooth.seatActive
            )

            Spacer()
        }
        .padding(24)
    }
}

#Preview {
    ContentView()
}
