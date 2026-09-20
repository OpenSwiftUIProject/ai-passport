import OpenSwiftUI

nonisolated(unsafe) private var host: EmbeddedViewHost<ContentView>?

@_cdecl("swift_preview_init")
func initializePreview() {
    guard passport_scene_begin(73) else { return }
    host = EmbeddedViewHost { ContentView() }
    if let host { renderPassportScene(host) }
}

@_cdecl("swift_preview_button")
func sendPreviewButton(_ value: Int32) {
    guard let host else { return }
    switch value {
    case 0: host.send(.upArrow)
    case 1: host.send(.downArrow)
    case 2: host.send(.select)
    default: return
    }
    if host.needsRender { renderPassportScene(host) }
}

@_cdecl("swift_preview_tick")
func tickPreview(_ milliseconds: UInt32) {
    guard let host else { return }
    if host.isAnimating { host.advanceAnimation(byMilliseconds: milliseconds) }
    if host.needsRender { renderPassportScene(host) }
}
