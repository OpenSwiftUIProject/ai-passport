option(PASSPORT_2048_SOAK "Run a bounded on-device 2048 stress diagnostic" OFF)

idf_component_register(
    SRCS "main.c"
         "physical_input.c"
         "ui_pixel.c"
         "ui_pixel_math.c"
         "demo_display.c"
         "demo_button.c"
         "demo_audio.c"
         "demo_battery.c"
         "demo_radio.c"
         "demo_wifi.c"
         "demo_ble.c"
         "demo_low_power.c"
         "demo_swift.c"
         "demo_openswiftui.c"
         "openswiftui_scene.c"
         "demo_2048.c"
         "demo_2048_soak.c"
         "screen_capture.c"
         "screen_protocol.c"
    INCLUDE_DIRS "."
    REQUIRES bsp bt esp_event esp_hw_support esp_netif esp_timer esp_wifi nvs_flash idf_swift esp_driver_usb_serial_jtag
)

set(passport_diagnostic_swift)
if(PASSPORT_2048_SOAK)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE PASSPORT_2048_SOAK=1)
    list(APPEND passport_diagnostic_swift "swift/Game2048/Game2048Soak.swift")
endif()

idf_component_register_swift(${COMPONENT_LIB}
    BRIDGING_HEADER "${CMAKE_CURRENT_LIST_DIR}/swift/PassportBridge.h"
    SRCS ${passport_diagnostic_swift} "swift/PassportCore/PassportState.swift" "swift/PassportDemo.swift"
         "swift/ContentView.swift" "swift/PassportSceneSink.swift" "swift/PassportContent.swift"
         "swift/PassportCore/Game2048.swift"
         "swift/Game2048/Game2048View.swift" "swift/Game2048/Game2048BoardView.swift"
         "swift/Game2048/Game2048RowView.swift" "swift/Game2048/Game2048TileView.swift"
         "swift/Game2048/Game2048ScoreView.swift" "swift/Game2048/Game2048ControlsView.swift"
         "swift/Game2048/Passport2048.swift"
         "swift/Game2048/Game2048TileLayout.swift" "swift/Game2048/Game2048AnimatedTile.swift"
         "swift/Game2048/Game2048TilesView.swift"
)

if(DEFINED ENV{OPENSWIFTUI_SOURCE_DIR})
    set(osui_default "$ENV{OPENSWIFTUI_SOURCE_DIR}")
else()
    set(osui_default "${PROJECT_DIR}/../framework/OpenSwiftUI")
endif()
set(OPENSWIFTUI_SOURCE_DIR "${osui_default}" CACHE PATH "OpenSwiftUI Embedded worktree")
if(NOT EXISTS "${OPENSWIFTUI_SOURCE_DIR}/Embedded/idf.cmake")
    message(FATAL_ERROR "OpenSwiftUI Embedded profile missing: set OPENSWIFTUI_SOURCE_DIR; see README.md")
endif()
include("${OPENSWIFTUI_SOURCE_DIR}/Embedded/idf.cmake")
openswiftui_link_embedded(${COMPONENT_LIB})
