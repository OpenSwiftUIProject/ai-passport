import OpenSwiftUI

/// The empty cells stay in place while identified foreground tiles move.
struct Game2048RowView: View {
    var body: some View {
        HStack(spacing: 4) {
            Game2048TileView(value: 0)
            Game2048TileView(value: 0)
            Game2048TileView(value: 0)
            Game2048TileView(value: 0)
        }
    }
}
