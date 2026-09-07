import OpenSwiftUI

struct Game2048TilesView: View {
    let tiles: [Game2048.Motion]
    let spawnedID: UInt32
    var body: some View {
        Game2048TileLayout(tiles: tiles) {
            Game2048AnimatedTile(tile: tiles[0], spawnedID: spawnedID)
            Game2048AnimatedTile(tile: tiles[1], spawnedID: spawnedID)
            Game2048AnimatedTile(tile: tiles[2], spawnedID: spawnedID)
            Game2048AnimatedTile(tile: tiles[3], spawnedID: spawnedID)
            Game2048AnimatedTile(tile: tiles[4], spawnedID: spawnedID)
            Game2048AnimatedTile(tile: tiles[5], spawnedID: spawnedID)
            Game2048AnimatedTile(tile: tiles[6], spawnedID: spawnedID)
            Game2048AnimatedTile(tile: tiles[7], spawnedID: spawnedID)
            Game2048AnimatedTile(tile: tiles[8], spawnedID: spawnedID)
            Game2048AnimatedTile(tile: tiles[9], spawnedID: spawnedID)
            Game2048AnimatedTile(tile: tiles[10], spawnedID: spawnedID)
            Game2048AnimatedTile(tile: tiles[11], spawnedID: spawnedID)
            Game2048AnimatedTile(tile: tiles[12], spawnedID: spawnedID)
            Game2048AnimatedTile(tile: tiles[13], spawnedID: spawnedID)
            Game2048AnimatedTile(tile: tiles[14], spawnedID: spawnedID)
            Game2048AnimatedTile(tile: tiles[15], spawnedID: spawnedID)
        }
    }
}
