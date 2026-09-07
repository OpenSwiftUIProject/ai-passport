import OpenSwiftUI

struct Game2048ControlsView: View {
    let axis: Game2048.Axis
    let status: Game2048.Status

    private var statusText: StaticString {
        switch status {
        case .won: "2048! YOU WIN"
        case .lost: "NO MOVES LEFT"
        case .playing: axis == .vertical ? "VERT  UP:^ DOWN:v" : "HORIZ UP:< DOWN:>"
        }
    }

    var body: some View {
        VStack(spacing: 0) {
            Text(statusText).foregroundStyle(axis == .vertical ? .white : .yellow)
            Text(status == .playing ? "OK:axis  HOLD:back" : "OK:new  HOLD:back")
        }
    }
}
