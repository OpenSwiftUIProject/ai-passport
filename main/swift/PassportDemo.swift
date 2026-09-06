// Compiled together with PassportCore by idf_component_register_swift().
// Startup initializes the battery snapshot before callbacks can enter this page;
// subsequent page state is serialized by the menu's BSP LVGL lock.
nonisolated(unsafe) private var state = PassportState()
nonisolated(unsafe) private var bootBatteryPercent: Int32 = -1

@_cdecl("passport_swift_prepare")
func preparePassport() {
    // This BSP call may block on I2C. Only call during app_main initialization.
    bootBatteryPercent = bsp_battery_soc()
    print("Embedded Swift ready on FoloToy ESP32-C3")
}

@_cdecl("passport_swift_enter")
func enterPassport() {
    state = PassportState()
    passport_ui_create(bootBatteryPercent)
    renderPassport()
}

@_cdecl("passport_swift_exit")
func exitPassport() {
    // Other hardware demos expect the baseline's full backlight.
    bsp_display_backlight(100)
    passport_ui_destroy()
}

@_cdecl("passport_swift_action")
func handlePassportAction(_ action: Int32) {
    switch action {
    case Int32(PASSPORT_ACTION_UP.rawValue): state.apply(.increment)
    case Int32(PASSPORT_ACTION_DOWN.rawValue): state.apply(.decrement)
    case Int32(PASSPORT_ACTION_OK.rawValue): state.apply(.toggleBacklight)
    default: return
    }
    renderPassport()
}

private func renderPassport() {
    // Direct Swift -> FoloToy C BSP call, compiled for the real ESP32-C3 target.
    bsp_display_backlight(state.backlight)
    passport_ui_render(state.count, state.backlight)
}
