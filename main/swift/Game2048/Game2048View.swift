import OpenSwiftUI

/// All game state belongs to this retained root; child views are value projections.
struct Game2048View: View {
    @State private var game: Game2048

    @State private var moving = false
    let animationsEnabled: Bool

    init(game: Game2048, animationsEnabled: Bool = true) {
        _game = State(wrappedValue: game)
        self.animationsEnabled = animationsEnabled
    }

    /// Called after the host has presented the last movement frame.
    func animationDidFinish() {
        guard moving else { return }
        withAnimation(animationsEnabled ? .easeOut(duration: 0.16) : nil) { moving = false }
    }

    var body: some View {
        VStack(spacing: 2) {
            HStack(spacing: 4) {
                Text("SCORE").foregroundStyle(.white)
                Game2048ScoreView(score: game.score)
            }
            .frame(width: 216, height: 16, alignment: .topLeading)

            Game2048BoardView(game: game, moving: moving)
            Game2048ControlsView(axis: game.axis, status: game.status)
        }
        .onPhyicButton(.up) { handle(.up) }
        .onPhyicButton(.down) { handle(.down) }
        .onPhyicButton(.ok) { handle(.confirm) }
    }

    private func handle(_ action: Game2048.Action) {
        var next = game
        next.apply(action)
        guard next != game else { return }
        let moves = action != .confirm && next.cells != game.cells
        withAnimation(moves && animationsEnabled ? .linear(duration: 0.24) : nil) {
            game = next
            moving = moves && animationsEnabled
        }
    }
}
