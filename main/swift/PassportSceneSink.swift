import OpenSwiftUI

/// C owns all LVGL objects; the menu holds its lock for this entire traversal.
struct PassportSceneSink: EmbeddedRenderSink {
    private(set) var succeeded = true
    let originX: Int32
    let originY: Int32

    mutating func measureImage(_ name: StaticString) -> EmbeddedSize {
        guard succeeded else { return .zero }
        var width: Int32 = 0, height: Int32 = 0
        succeeded = passport_scene_measure_image(name.utf8Start, UInt32(name.utf8CodeUnitCount), &width, &height)
        return succeeded ? EmbeddedSize(width: width, height: height) : .zero
    }

    mutating func measureText(_ text: StaticString, proposal: ProposedViewSize) -> EmbeddedSize {
        guard succeeded else { return .zero }
        var width: Int32 = 0, height: Int32 = 0
        succeeded = passport_scene_measure_text(text.utf8Start, UInt32(text.utf8CodeUnitCount),
                                                proposal.width ?? -1, &width, &height)
        return succeeded ? EmbeddedSize(width: width, height: height) : .zero
    }

    mutating func fill(_ rect: EmbeddedRect, color: Color) {
        if succeeded {
            succeeded = passport_scene_fill(rect.x - originX, rect.y - originY, rect.width, rect.height, color.rgb, color.alpha)
        }
    }

    mutating func image(_ name: StaticString, in rect: EmbeddedRect) {
        if succeeded {
            succeeded = passport_scene_image(rect.x - originX, rect.y - originY, rect.width, rect.height,
                                               name.utf8Start, UInt32(name.utf8CodeUnitCount))
        }
    }

    mutating func text(_ text: StaticString, in rect: EmbeddedRect, color: Color) {
        if succeeded {
            succeeded = passport_scene_text(rect.x - originX, rect.y - originY, rect.width, rect.height,
                                              text.utf8Start, UInt32(text.utf8CodeUnitCount), color.rgb, color.alpha)
        }
    }
}

// Both pages use the same measured root and C sink, with page-specific insets.
func renderPassportScene<Content: View>(_ host: EmbeddedViewHost<Content>) {
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
