import OpenSwiftUI

@_cdecl("passport_content_enter")
func enterPassportContent() {
    guard passport_scene_begin(passportBatterySnapshot()) else { return }
    var configuration = passport_scene_geometry_t()
    guard passport_scene_get_geometry(&configuration) else {
        passport_scene_end(false)
        return
    }
    let geometry = RootGeometry(
        screenSize: EmbeddedSize(width: configuration.screen_width, height: configuration.screen_height),
        safeAreaInsets: EdgeInsets(top: configuration.top, leading: configuration.leading,
                                  bottom: configuration.bottom, trailing: configuration.trailing)
    )
    var sink = PassportSceneSink(originX: geometry.contentBounds.x, originY: geometry.contentBounds.y)
    EmbeddedRenderer.render(ContentView(), rootGeometry: geometry, to: &sink)
    passport_scene_end(sink.succeeded)
}
