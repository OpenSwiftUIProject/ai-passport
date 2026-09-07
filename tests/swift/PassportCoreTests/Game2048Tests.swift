import XCTest
@testable import PassportCore

final class Game2048Tests: XCTestCase {
    private func checkMove(_ cells: [UInt16], _ direction: Game2048.Direction,
                           _ expected: [UInt16], score: UInt32,
                           file: StaticString = #filePath, line: UInt = #line) {
        var game = Game2048(cells: cells, seed: 17)
        XCTAssertTrue(game.move(direction), file: file, line: line)
        XCTAssertEqual(game.score, score, file: file, line: line)
        var spawned: [UInt16] = []
        for index in cells.indices {
            if expected[index] != 0 {
                XCTAssertEqual(game.cells[index], expected[index], file: file, line: line)
            } else if game.cells[index] != 0 {
                spawned.append(game.cells[index])
            }
        }
        XCTAssertEqual(spawned.count, 1, file: file, line: line)
        XCTAssertTrue(spawned.allSatisfy { $0 == 2 || $0 == 4 }, file: file, line: line)
    }

    func testOpeningHasTwoDistinctTilesAndRepeatableSeed() {
        for seed in UInt32(0)..<100 {
            let game = Game2048(seed: seed)
            XCTAssertEqual(game, Game2048(seed: seed))
            XCTAssertEqual(game.cells.filter { $0 != 0 }.count, 2)
            XCTAssertTrue(game.cells.allSatisfy { $0 == 0 || $0 == 2 || $0 == 4 })
            XCTAssertEqual(game.axis, .vertical)
            XCTAssertEqual(game.status, .playing)
            XCTAssertEqual(game.score, 0)
        }
    }

    func testEachPairMergesOnlyOnceAndGapsAreCompacted() {
        checkMove([2,2,2,2, 2,2,4,0, 2,0,2,2, 4,4,2,2], .left,
                  [4,4,0,0, 4,4,0,0, 4,2,0,0, 8,4,0,0], score: 28)
    }

    func testRightMergesFromTheRightEdge() {
        checkMove([2,2,2,0, 4,4,4,4, 2,0,2,4, 0,0,0,0], .right,
                  [0,0,2,4, 0,0,8,8, 0,0,4,4, 0,0,0,0], score: 24)
    }

    func testUpMergesColumns() {
        checkMove([2,4,0,2, 2,4,0,0, 2,8,4,2, 0,8,4,2], .up,
                  [4,8,8,4, 2,16,0,2, 0,0,0,0, 0,0,0,0], score: 40)
    }

    func testDownMergesFromBottomEdge() {
        checkMove([2,4,0,2, 2,4,0,0, 2,8,4,2, 0,8,4,2], .down,
                  [0,0,0,0, 0,0,0,0, 2,8,0,2, 4,16,8,4], score: 40)
    }

    func testInvalidMovePreservesBoardScoreAndRandomState() {
        var game = Game2048(cells: [2,4,8,16, 0,0,0,0, 0,0,0,0, 0,0,0,0])
        let before = game
        XCTAssertFalse(game.move(.left))
        XCTAssertEqual(game, before)
    }

    func testAxisSwitchDoesNotMoveOrSpawnAndPersists() {
        var game = Game2048(seed: 17)
        let before = game
        game.apply(.confirm)
        XCTAssertEqual(game.axis, .horizontal)
        XCTAssertEqual(game.cells, before.cells)
        XCTAssertEqual(game.score, before.score)
        game.apply(.confirm)
        XCTAssertEqual(game, before)
        var expected = game
        expected.move(.up)
        game.apply(.up)
        XCTAssertEqual(game, expected)
        game.apply(.confirm)
        expected = game
        expected.move(.right)
        game.apply(.down)
        XCTAssertEqual(game, expected)
        XCTAssertEqual(game.axis, .horizontal)
        expected = game
        expected.move(.left)
        game.apply(.up)
        XCTAssertEqual(game, expected)
    }

    func testFullBoardCanStillMergeOnEitherAxis() {
        let blocked: [UInt16] = [2,4,2,4, 4,2,4,2, 2,4,2,4, 4,2,4,2]
        XCTAssertEqual(Game2048(cells: blocked).status, .lost)
        var horizontal = blocked
        horizontal[1] = horizontal[0]
        XCTAssertEqual(Game2048(cells: horizontal).status, .playing)
        var vertical = blocked
        vertical[4] = vertical[0]
        XCTAssertEqual(Game2048(cells: vertical).status, .playing)
    }

    func testLastEmptyCellCanEndTheGame() {
        // Both possible spawn values are blocked at the sole empty cell after UP.
        var game = Game2048(cells: [0,2,4,2, 8,4,2,4, 2,2,4,2, 8,16,2,4])
        XCTAssertTrue(game.move(.up))
        XCTAssertEqual(game.cells.filter { $0 == 0 }.count, 0)
        XCTAssertEqual(game.status, .lost)
    }

    func testWinStopsMovesAndConfirmStartsFreshGame() {
        var game = Game2048(cells: [1024,1024,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0])
        XCTAssertTrue(game.move(.left))
        XCTAssertEqual(game.status, .won)
        XCTAssertEqual(game.score, 2048)
        let won = game
        game.apply(.down)
        XCTAssertEqual(game, won)
        game.apply(.confirm)
        XCTAssertEqual(game.status, .playing)
        XCTAssertEqual(game.score, 0)
        XCTAssertEqual(game.axis, .vertical)
        XCTAssertEqual(game.cells.filter { $0 != 0 }.count, 2)
    }

    func testLossIgnoresDirectionsAndConfirmRestarts() {
        var game = Game2048(cells: [2,4,2,4, 4,2,4,2, 2,4,2,4, 4,2,4,2])
        let lost = game
        game.apply(.up)
        game.apply(.down)
        XCTAssertEqual(game, lost)
        game.apply(.confirm)
        XCTAssertEqual(game.status, .playing)
        XCTAssertEqual(game.cells.filter { $0 != 0 }.count, 2)
    }

    func testLongSequencesConserveTileMassAndOnlySpawnOnChangedMoves() {
        let directions: [Game2048.Direction] = [.left, .down, .right, .up]
        for seed in UInt32(1)...20 {
            var game = Game2048(seed: seed)
            for step in 0..<500 {
                let before = game
                if game.move(directions[(step + Int(seed)) % 4]) {
                    let added = game.cells.reduce(0) { $0 + Int($1) } - before.cells.reduce(0) { $0 + Int($1) }
                    XCTAssertTrue(added == 2 || added == 4)
                    XCTAssertGreaterThanOrEqual(game.score, before.score)
                    XCTAssertEqual(game.score % 4, 0)
                } else {
                    XCTAssertEqual(game, before)
                }
                XCTAssertEqual(game.cells.count, 16)
                XCTAssertTrue(game.cells.allSatisfy { $0 == 0 || ($0 >= 2 && $0 <= 2048 && $0 & ($0 - 1) == 0) })
            }
        }
    }
    func testMergeMotionPreservesBothSourcesAndCreatesNewIdentity() {
        var game = Game2048(cells: [2,0,2,4, 0,0,0,0, 0,0,0,0, 0,0,0,0])
        let ids = game.tileIDs
        XCTAssertTrue(game.move(.left))
        XCTAssertEqual(game.motions[0].id, ids[0])
        XCTAssertEqual(game.motions[2].id, ids[2])
        XCTAssertEqual(game.motions[0].value, 2)
        XCTAssertEqual(game.motions[2].value, 2)
        XCTAssertEqual(game.motions[0].destination, 0)
        XCTAssertEqual(game.motions[2].destination, 0)
        XCTAssertEqual(game.motions[3].destination, 1)
        XCTAssertEqual(game.tileIDs[1], ids[3])
        XCTAssertFalse(ids.contains(game.tileIDs[0]))
        XCTAssertEqual(game.cells[0], 4)
        let occupiedIDs = game.tileIDs.filter { $0 != 0 }
        XCTAssertEqual(Set(occupiedIDs).count, occupiedIDs.count)
    }

    func testMotionDestinationsTrackAllDirections() {
        let directions: [Game2048.Direction] = [.up, .down, .left, .right]
        let destinations = [1, 13, 4, 7]
        for (direction, destination) in zip(directions, destinations) {
            var cells = Array(repeating: UInt16(0), count: 16)
            cells[5] = 8
            var game = Game2048(cells: cells)
            let id = game.tileIDs[5]
            XCTAssertTrue(game.move(direction))
            XCTAssertEqual(game.motions[5].source, 5)
            XCTAssertEqual(game.motions[5].destination, destination)
            XCTAssertEqual(game.tileIDs[destination], id)
        }
    }

}
