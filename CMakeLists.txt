cmake_minimum_required(VERSION 3.16)
project(endgame-op1w VERSION 0.1.0 LANGUAGES CXX)

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

add_library(egg STATIC
    src/egg/device.cpp
    src/egg/settings.cpp
)
target_include_directories(egg PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(egg PUBLIC PkgConfig::HIDAPI)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(EGG_WARNINGS -Wall -Wextra)
else()
    set(EGG_WARNINGS /W4)
endif()
target_compile_options(egg PRIVATE ${EGG_WARNINGS})

# -------------------------------------------------------------------- cli ---

add_executable(egg-cli src/cli/main.cpp)
target_link_libraries(egg-cli PRIVATE egg)
target_compile_options(egg-cli PRIVATE ${EGG_WARNINGS})

# -------------------------------------------------------------------- gui ---

find_package(Qt6 6.2 COMPONENTS Widgets QUIET)
if(Qt6_FOUND)
    set(CMAKE_AUTOMOC ON)
    add_executable(egg-gui
        src/gui/main.cpp
        src/gui/MainWindow.cpp
    )
    target_link_libraries(egg-gui PRIVATE egg Qt6::Widgets)
    target_compile_options(egg-gui PRIVATE ${EGG_WARNINGS})
    install(TARGETS egg-gui RUNTIME DESTINATION bin)
else()
    message(STATUS "Qt6 Widgets not found - building egg-cli only")
endif()

install(TARGETS egg-cli RUNTIME DESTINATION bin)
install(FILES udev/70-endgamegear.rules
        DESTINATION lib/udev/rules.d)
