// swift-tools-version: 6.0
import PackageDescription

// Host tests compile the same hardware-independent source used by ESP-IDF.
// The firmware itself is built by CMake, not SwiftPM.
let package = Package(
    name: "PassportSwift",
    products: [.library(name: "PassportCore", targets: ["PassportCore"])],
    targets: [
        .target(name: "PassportCore", path: "main/swift/PassportCore"),
        .testTarget(name: "PassportCoreTests", dependencies: ["PassportCore"],
                    path: "tests/swift/PassportCoreTests"),
    ]
)
