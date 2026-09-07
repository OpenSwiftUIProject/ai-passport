import OpenSwiftUI

struct Game2048AnimatedTile: View {
    let tile: Game2048.Motion
    let spawnedID: UInt32
    var body: some View {
        if tile.value == 0 {
            Color.clear.frame(width: 42, height: 42)
        } else {
            Game2048TileView(value: tile.value)
                .transition(tile.id == spawnedID ? .scale.combined(with: .opacity) : .scale)
                .id(tile.id)
        }
    }
}
