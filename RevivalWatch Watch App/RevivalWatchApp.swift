import SwiftUI

@main
struct RevivalWatch_Watch_AppApp: App {

    @StateObject private var watchSession = WatchSessionManager.shared

    var body: some Scene {
        WindowGroup {
            ContentView()
        }
    }
}
