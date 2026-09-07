import OpenSwiftUI

/// The Embedded Text API accepts StaticString. Six fixed digit slots display
/// the score without Foundation, dynamic strings, or a separate C label.
struct Game2048ScoreView: View {
    let score: UInt32

    private func digit(_ place: UInt32) -> StaticString {
        if place > 1 && score < place { return " " }
        switch score / place % 10 {
        case 0: return "0"
        case 1: return "1"
        case 2: return "2"
        case 3: return "3"
        case 4: return "4"
        case 5: return "5"
        case 6: return "6"
        case 7: return "7"
        case 8: return "8"
        default: return "9"
        }
    }

    var body: some View {
        HStack(spacing: 0) {
            Text(digit(100_000)).frame(width: 9)
            Text(digit(10_000)).frame(width: 9)
            Text(digit(1_000)).frame(width: 9)
            Text(digit(100)).frame(width: 9)
            Text(digit(10)).frame(width: 9)
            Text(digit(1)).frame(width: 9)
        }
    }
}
