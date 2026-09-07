// Compiled only by the explicit device diagnostic and host previews.
// Production firmware has no autonomous play or input-injection entry point.
@_cdecl("passport_2048_soak_setup")
func setUpGame2048Soak(_ scenario: UInt32) {
    exitPassport2048()
    passport_scene_destroy()
    if scenario % 3 == 0 {
        startPassport2048(Game2048(cells: [2,4,8,16, 32,64,128,256, 512,1024,2,4, 8,16,32,32], seed: scenario + 1))
    } else if scenario % 3 == 1 {
        startPassport2048(Game2048(cells: [2,2,4,4, 8,8,16,16, 32,32,64,64, 128,128,256,256], seed: scenario + 1))
    } else {
        startPassport2048(Game2048(seed: scenario + 1))
    }
}
