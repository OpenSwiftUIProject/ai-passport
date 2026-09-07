import OpenSwiftUI

nonisolated(unsafe) private var gameHost: EmbeddedViewHost<Game2048View>?
nonisolated(unsafe) private var pendingActions: [PhysicalButton] = []

@_cdecl("passport_2048_enter")
func enterPassport2048(_ seed: UInt32) { startPassport2048(Game2048(seed: seed)) }

func startPassport2048(_ game: Game2048) {
    guard gameHost == nil, passport_scene_begin_2048(passportBatterySnapshot()) else { return }
    let clockReady = passport_scene_start_clock { elapsed in tickPassport2048(elapsed) }
    gameHost = EmbeddedViewHost { Game2048View(game: game, animationsEnabled: clockReady) }
    pendingActions.removeAll(keepingCapacity: true)
    renderPassport2048()
}

@_cdecl("passport_2048_exit")
func exitPassport2048() {
    passport_scene_stop_clock()
    pendingActions.removeAll(keepingCapacity: false)
    gameHost = nil
}

@_cdecl("passport_2048_action")
func handlePassport2048Action(_ action: Int32) {
    guard let host = gameHost else { return }
    let button: PhysicalButton
    switch action {
    case Int32(PASSPORT_ACTION_UP.rawValue): button = .up
    case Int32(PASSPORT_ACTION_DOWN.rawValue): button = .down
    case Int32(PASSPORT_ACTION_OK.rawValue): button = .ok
    default: return
    }
    if host.isAnimating {
        // Bounded FIFO; rapid axis changes retain their ordering with moves.
        if pendingActions.count < 8 { pendingActions.append(button) }
        else { passport_scene_input_overflow() }
    } else {
        host.send(button)
        if host.needsRender { renderPassport2048() }
        if host.isAnimating { passport_scene_clock_begin() }
    }
}

@_cdecl("passport_2048_tick")
func tickPassport2048(_ elapsed: UInt32) {
    guard let host = gameHost else { return }
    let wasAnimating = host.isAnimating
    host.advanceAnimation(byMilliseconds: elapsed)
    if host.needsRender { renderPassport2048() }
    if wasAnimating && !host.isAnimating {
        host.update { $0.animationDidFinish() }
        if host.needsRender { renderPassport2048() }
        if host.isAnimating { passport_scene_clock_phase() }
        else { passport_scene_clock_report() }
    }
    while !host.isAnimating && !pendingActions.isEmpty {
        host.send(pendingActions.removeFirst())
        if host.needsRender { renderPassport2048() }
        if host.isAnimating { passport_scene_clock_begin() }
    }
}

private func renderPassport2048() {
    guard let host = gameHost else { return }
    renderPassportScene(host)
}
