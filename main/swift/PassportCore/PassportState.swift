/// Small, allocation-free application state shared by host tests and firmware.
public struct PassportState: Equatable {
    public enum Action { case increment, decrement, toggleBacklight }

    public private(set) var count: Int32 = 0
    public private(set) var backlight: UInt8 = 100
    public static let maximumCount: Int32 = 999

    public init() {}

    /// Saturate rather than wrap so repeated physical input never overflows.
    public mutating func apply(_ action: Action) {
        switch action {
        case .increment:
            if count < Self.maximumCount { count += 1 }
        case .decrement:
            if count > 0 { count -= 1 }
        case .toggleBacklight:
            backlight = backlight == 100 ? 25 : 100
        }
    }
}
