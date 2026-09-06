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
    var configuration = passport_scene_geometry_t()
    guard passport_scene_get_geometry(&configuration), passport_scene_reset() else {
        passport_scene_end(false)
        host.invalidate()
        return
    }
    let geometry = RootGeometry(
        screenSize: EmbeddedSize(width: configuration.screen_width, height: configuration.screen_height),
        safeAreaInsets: EdgeInsets(top: configuration.top, leading: configuration.leading,
                                  bottom: configuration.bottom, trailing: configuration.trailing)
    )
    var sink = PassportSceneSink(originX: geometry.contentBounds.x, originY: geometry.contentBounds.y)
    host.render(rootGeometry: geometry, to: &sink)
    passport_scene_end(sink.succeeded)
    if !sink.succeeded { host.invalidate() }
}
