import OpenSwiftUI

/// A 4x4 measure/place layout; animation interpolates its resolved rectangles.
struct Game2048TileLayout: Layout {
    let tiles: [Game2048.Motion]
    func sizeThatFits<Content: View, Sink: EmbeddedRenderSink>(proposal: ProposedViewSize, subviews: inout LayoutSubviews<Content, Sink>, cache: inout ()) -> EmbeddedSize {
        .init(width: 180, height: 180)
    }
    func placeSubviews<Content: View, Sink: EmbeddedRenderSink>(in bounds: EmbeddedRect, proposal: ProposedViewSize, subviews: inout LayoutSubviews<Content, Sink>, cache: inout ()) {
        precondition(tiles.count == 16 && subviews.count == 16)
        for index in 0..<16 {
            let cell = tiles[index].destination
            subviews.place(index, x: bounds.x + Int32(cell % 4) * 46,
                           y: bounds.y + Int32(cell / 4) * 46,
                           proposal: .init(width: 42, height: 42))
        }
    }
}
