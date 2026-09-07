import OpenSwiftUI

// All access is serialized by the platform UI context. The host retains the
// ContentView value and its @State boxes until page exit.
nonisolated(unsafe) private var contentHost: EmbeddedViewHost<ContentView>?

@_cdecl("passport_content_enter")
func enterPassportContent() {
    guard contentHost == nil else { return }
    guard passport_scene_begin(passportBatterySnapshot()) else { return }
    contentHost = EmbeddedViewHost { ContentView() }
    renderPassportContent()
}

@_cdecl("passport_content_exit")
func exitPassportContent() {
    contentHost = nil
}

@_cdecl("passport_content_action")
func handlePassportContentAction(_ action: Int32) {
    guard let host = contentHost else { return }
    let button: PhysicalButton
    switch action {
    case Int32(PASSPORT_ACTION_UP.rawValue): button = .up
    case Int32(PASSPORT_ACTION_DOWN.rawValue): button = .down
    case Int32(PASSPORT_ACTION_OK.rawValue): button = .ok
    default: return
    }
    host.send(button)
    if host.needsRender { renderPassportContent() }
}

private func renderPassportContent() {
    guard let host = contentHost else { return }
    renderPassportScene(host)
}
