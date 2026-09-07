import OpenSwiftUI

struct Game2048BoardView: View {
    let game: Game2048
    var moving = false

    var body: some View {
        // The fixed background uses stacks. A Layout places moving tiles at
        // destination cells, including two separate tiles entering one merge.
        Game2048TilesView(tiles: moving ? game.motions : game.settledTiles, spawnedID: game.spawnedTileID)
            .background(VStack(spacing: 4) {
                Game2048RowView()
                Game2048RowView()
                Game2048RowView()
                Game2048RowView()
            })
            .padding(4)
            .background(game.axis == .vertical
                ? Color(red: 0.09, green: 0.18, blue: 0.27)
                : Color(red: 0.40, green: 0.24, blue: 0.09))
    }
}
