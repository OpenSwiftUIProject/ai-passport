/// Standard 4x4 2048 rules, independent of the renderer and board drivers.
/// A game ends at 2048 or when no move remains; OK starts the next game.
struct Game2048: Equatable {
    enum Direction { case up, down, left, right }
    enum Axis { case vertical, horizontal }
    enum Status { case playing, won, lost }
    enum Action { case up, down, confirm }

    struct Motion: Equatable {
        let id: UInt32
        let value: UInt16
        let source: Int
        var destination: Int
    }
    private(set) var tileIDs = Array(repeating: UInt32(0), count: 16)
    private(set) var motions: [Motion] = []
    private var nextTileID: UInt32 = 1
    private(set) var spawnedTileID: UInt32 = 0
    private(set) var cells: [UInt16]
    private(set) var score: UInt32 = 0
    private(set) var axis: Axis = .vertical
    private(set) var status: Status = .playing
    private var randomState: UInt32

    init(seed: UInt32) {
        cells = Array(repeating: 0, count: 16)
        randomState = seed == 0 ? 0x2048 : seed
        spawnTile()
        spawnTile()
    }

    /// Restore a board snapshot; also makes exact rule scenarios testable.
    init(cells: [UInt16], score: UInt32 = 0, seed: UInt32 = 1) {
        precondition(cells.count == 16)
        precondition(cells.allSatisfy { $0 == 0 || ($0 >= 2 && $0 <= 2048 && $0 & ($0 - 1) == 0) })
        self.cells = cells
        self.score = score
        randomState = seed == 0 ? 0x2048 : seed
        for index in cells.indices where cells[index] != 0 { tileIDs[index] = allocateTileID() }
        updateStatus()
    }

    var settledTiles: [Motion] {
        cells.indices.map { .init(id: tileIDs[$0], value: cells[$0], source: $0, destination: $0) }
    }

    subscript(row: Int, column: Int) -> UInt16 { cells[row * 4 + column] }

    mutating func apply(_ action: Action) {
        if action == .confirm {
            if status == .playing {
                axis = axis == .vertical ? .horizontal : .vertical
            } else {
                self = Game2048(seed: nextRandom())
            }
        } else if status == .playing {
            let direction: Direction = axis == .vertical
                ? (action == .up ? .up : .down)
                : (action == .up ? .left : .right)
            move(direction)
        }
    }

    @discardableResult
    mutating func move(_ direction: Direction) -> Bool {
        guard status == .playing else { return false }
        let before = self
        motions = settledTiles
        for line in 0..<4 {
            var compact: [Motion] = []
            for offset in 0..<4 {
                let cell = index(line: line, offset: offset, direction: direction)
                if cells[cell] != 0 { compact.append(motions[cell]) }
                cells[cell] = 0
                tileIDs[cell] = 0
            }
            var source = 0, destination = 0
            while source < compact.count {
                let tile = compact[source]
                let cell = index(line: line, offset: destination, direction: direction)
                motions[tile.source].destination = cell
                if source + 1 < compact.count && compact[source + 1].value == tile.value {
                    motions[compact[source + 1].source].destination = cell
                    cells[cell] = tile.value * 2
                    tileIDs[cell] = allocateTileID()
                    score += UInt32(tile.value * 2)
                    source += 2 // A newly merged tile cannot merge again this move.
                } else {
                    cells[cell] = tile.value
                    tileIDs[cell] = tile.id
                    source += 1
                }
                destination += 1
            }
        }
        guard cells != before.cells else { self = before; return false }
        spawnTile()
        updateStatus()
        return true
    }

    private func index(line: Int, offset: Int, direction: Direction) -> Int {
        switch direction {
        case .left: line * 4 + offset
        case .right: line * 4 + 3 - offset
        case .up: offset * 4 + line
        case .down: (3 - offset) * 4 + line
        }
    }

    private mutating func updateStatus() {
        if cells.contains(2048) { status = .won; return }
        if cells.contains(0) { status = .playing; return }
        for row in 0..<4 {
            for column in 0..<4 {
                if column < 3 && self[row, column] == self[row, column + 1] { status = .playing; return }
                if row < 3 && self[row, column] == self[row + 1, column] { status = .playing; return }
            }
        }
        status = .lost
    }

    private mutating func spawnTile() {
        let emptyCount = cells.reduce(0) { $0 + ($1 == 0 ? 1 : 0) }
        guard emptyCount > 0 else { return }
        var selected = Int(randomBelow(UInt32(emptyCount)))
        let value: UInt16 = randomBelow(10) == 0 ? 4 : 2
        for index in cells.indices where cells[index] == 0 {
            if selected == 0 {
                cells[index] = value
                tileIDs[index] = allocateTileID()
                spawnedTileID = tileIDs[index]
                return
            }
            selected -= 1
        }
    }

    private mutating func allocateTileID() -> UInt32 {
        precondition(nextTileID < UInt32.max)
        defer { nextTileID += 1 }
        return nextTileID
    }

    /// Seeded once by the platform. Repeatable tests require no hardware RNG.
    private mutating func nextRandom() -> UInt32 {
        randomState ^= randomState << 13
        randomState ^= randomState >> 17
        randomState ^= randomState << 5
        return randomState
    }

    private mutating func randomBelow(_ bound: UInt32) -> UInt32 {
        // Xorshift emits 1...UInt32.max. Reject the uneven tail before modulo.
        let limit = UInt32.max - UInt32.max % bound
        var value = nextRandom()
        while value > limit { value = nextRandom() }
        return (value - 1) % bound
    }
}
