import SwiftUI

@main
struct RevivalWatchApp: App {

    @StateObject private var watchSession = WatchSessionManager.shared

    var body: some Scene {
        WindowGroup {
            ContentView()
        }
    }
}
