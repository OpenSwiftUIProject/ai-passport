import OpenSwiftUI

// Host-only, explicitly staged boards for font/bounds/palette/end-state checks.
// Firmware compiles the same Game2048View, but never includes these fixtures.
@_cdecl("passport_2048_fixture")
func renderGame2048Fixture(_ scenario: Int32) {
    var cells: [UInt16] = [2,4,8,16, 32,64,128,256, 512,1024,2,4, 8,16,32,32]
    if scenario == 1 { cells[0] = 2048 }
    if scenario == 2 { cells = [2,4,2,4, 4,2,4,2, 2,4,2,4, 4,2,4,2] }
    precondition(passport_scene_begin_2048(73))
    let host = EmbeddedViewHost { Game2048View(game: Game2048(cells: cells, score: 123456)) }
    renderPassportScene(host)
}

// Installs an exact merge scenario into the real page host/queue/clock.
@_cdecl("passport_2048_animation_fixture")
func installGame2048AnimationFixture() {
    exitPassport2048()
    passport_scene_destroy()
    startPassport2048(Game2048(cells: [2,4,0,0, 0,0,0,0, 2,0,0,0, 0,4,0,0]))
}
