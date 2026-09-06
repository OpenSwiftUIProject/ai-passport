import XCTest
@testable import PassportCore

final class PassportStateTests: XCTestCase {
    func testNewPageStartsAtZeroWithFullBacklight() {
        let state = PassportState()
        XCTAssertEqual(state.count, 0)
        XCTAssertEqual(state.backlight, 100)
    }

    func testPhysicalInputSequence() {
        var state = PassportState()
        state.apply(.increment)
        state.apply(.increment)
        state.apply(.toggleBacklight)
        state.apply(.decrement)
        XCTAssertEqual(state.count, 1)
        XCTAssertEqual(state.backlight, 25)
        state.apply(.toggleBacklight)
        XCTAssertEqual(state.backlight, 100)
        XCTAssertEqual(state.count, 1)
    }

    func testRepeatedInputSaturatesAndCanReverseFromEachLimit() {
        var state = PassportState()
        for _ in 0..<2_000 { state.apply(.increment) }
        XCTAssertEqual(state.count, 999)
        state.apply(.decrement)
        XCTAssertEqual(state.count, 998)
        for _ in 0..<2_000 { state.apply(.decrement) }
        XCTAssertEqual(state.count, 0)
        state.apply(.increment)
        XCTAssertEqual(state.count, 1)
    }

}
