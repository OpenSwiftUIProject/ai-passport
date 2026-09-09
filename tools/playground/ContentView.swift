import OpenSwiftUI

/// RootGeometry supplies the screen proposal; stacks measure and place children.
struct ContentView: View {
    @State private var palette = 0
    @State private var showImage = true

    private var panelColor: Color {
        switch palette {
        case 1: Color(red: 0.14, green: 0.30, blue: 0.22)
        case 2: Color(red: 0.30, green: 0.16, blue: 0.34)
        default: Color(red: 0.14, green: 0.22, blue: 0.34)
        }
    }

    var body: some View {
        VStack(spacing: 12) {
            VStack(spacing: 0) {
                if showImage {
                    Image("spark")
                        .resizable()
                        .frame(width: 80, height: 80)
                }
            }
            .frame(width: 184, height: 112)
            .background(panelColor)

            VStack(spacing: 6) {
                Text("Hello, OpenSwiftUI!")
                Text("Swift on ESP32-C3")
                    .foregroundStyle(.yellow)
            }

            HStack(spacing: 16) {
                Color.red.frame(width: 32, height: 8)
                Color.green.frame(width: 32, height: 8)
                Color.blue.frame(width: 32, height: 8)
            }
        }
        .padding(12)
        .background(Color(red: 0.055, green: 0.09, blue: 0.16))
        .onPhyicButton(.up) { palette = (palette + 2) % 3 }
        .onPhyicButton(.down) { palette = (palette + 1) % 3 }
        .onPhyicButton(.ok) { showImage.toggle() }
    }
}
