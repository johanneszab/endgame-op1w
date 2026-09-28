cmake_minimum_required(VERSION 3.16)
project(vole VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE)
endif()

find_package(PkgConfig REQUIRED)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # The hidraw backend, not libusb: hidraw coexists with the kernel's usbhid
    # driver, so the mouse keeps working while we talk to it. This is not a
    # preference — do not relax it to plain "hidapi" here, which can resolve to
    # hidapi-libusb and detach the kernel driver.
    pkg_check_modules(HIDAPI REQUIRED IMPORTED_TARGET hidapi-hidraw)
else()
    # Elsewhere (MSYS2/mingw, macOS) there is one backend and it is named
    # plainly. Building off Linux is only ever a compile check: device
    # enumeration differs — Windows exposes one path per HID collection where
    # Linux exposes one node per interface (see re/PROTOCOL.md section 7) — so
    # runtime behaviour there says nothing about the Linux path.
    pkg_check_modules(HIDAPI REQUIRED IMPORTED_TARGET hidapi)
endif()

# ---------------------------------------------------------------- library ---

add_library(vole STATIC
    src/vole/device.cpp
    src/vole/settings.cpp
)
target_include_directories(vole PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(vole PUBLIC PkgConfig::HIDAPI)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(VOLE_WARNINGS -Wall -Wextra)
else()
    set(VOLE_WARNINGS /W4)
endif()
target_compile_options(vole PRIVATE ${VOLE_WARNINGS})

# -------------------------------------------------------------------- cli ---

add_executable(vole-cli src/cli/main.cpp)
target_link_libraries(vole-cli PRIVATE vole)
target_compile_options(vole-cli PRIVATE ${VOLE_WARNINGS})

# --------------------------------------------------------- firmware tool ---
#
# Its own binary, and deliberately not linked against vole: that library picks
# its hidraw node by usage page, which the dongle also matches. Nothing that
# writes firmware should be one refactor away from addressing the dongle.
# See re/firmware/FIRMWARE.md §6.

add_executable(vole-fw src/fw/main.cpp src/fw/flasher.cpp src/fw/pe.cpp)
target_include_directories(vole-fw PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/fw)
target_link_libraries(vole-fw PRIVATE PkgConfig::HIDAPI)
target_compile_options(vole-fw PRIVATE ${VOLE_WARNINGS})
install(TARGETS vole-fw RUNTIME DESTINATION bin)

# -------------------------------------------------------------------- gui ---

find_package(Qt6 6.2 COMPONENTS Widgets QUIET)
if(Qt6_FOUND)
    set(CMAKE_AUTOMOC ON)
    add_executable(vole-gui
        src/gui/main.cpp
        src/gui/MainWindow.cpp
    )
    target_link_libraries(vole-gui PRIVATE vole Qt6::Widgets)
    target_compile_options(vole-gui PRIVATE ${VOLE_WARNINGS})
    install(TARGETS vole-gui RUNTIME DESTINATION bin)
else()
    message(STATUS "Qt6 Widgets not found - building vole-cli only")
endif()

install(TARGETS vole-cli RUNTIME DESTINATION bin)
install(FILES udev/70-vole.rules
        DESTINATION lib/udev/rules.d)
