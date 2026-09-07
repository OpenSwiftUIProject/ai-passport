import OpenSwiftUI

struct Game2048TileView: View {
    let value: UInt16

    private var caption: StaticString {
        switch value {
        case 2: "2"
        case 4: "4"
        case 8: "8"
        case 16: "16"
        case 32: "32"
        case 64: "64"
        case 128: "128"
        case 256: "256"
        case 512: "512"
        case 1024: "1024"
        case 2048: "2048"
        default: ""
        }
    }

    private var tileColor: Color {
        switch value {
        case 0: Color(red: 0.25, green: 0.34, blue: 0.42)
        case 2: Color(red: 0.93, green: 0.90, blue: 0.85)
        case 4: Color(red: 0.93, green: 0.86, blue: 0.71)
        case 8: Color(red: 0.96, green: 0.67, blue: 0.40)
        case 16: Color(red: 0.96, green: 0.53, blue: 0.32)
        case 32: Color(red: 0.94, green: 0.39, blue: 0.29)
        case 64: Color(red: 0.87, green: 0.28, blue: 0.20)
        case 128: Color(red: 0.94, green: 0.77, blue: 0.29)
        case 256: Color(red: 0.93, green: 0.72, blue: 0.20)
        case 512: Color(red: 0.92, green: 0.66, blue: 0.14)
        case 1024: Color(red: 0.90, green: 0.60, blue: 0.08)
        default: Color(red: 1.0, green: 0.80, blue: 0.0)
        }
    }

    var body: some View {
        Text(caption)
            .foregroundStyle(value >= 8 && value <= 64 ? .white : Color(red: 0.12, green: 0.16, blue: 0.19))
            .frame(width: 42, height: 42)
            .background(tileColor)
    }
}
