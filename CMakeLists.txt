cmake_minimum_required(VERSION 3.16)
project(endgame-op1w VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE)
endif()

find_package(PkgConfig REQUIRED)
# The hidraw backend, not libusb: hidraw coexists with the kernel's usbhid
# driver, so the mouse keeps working while we talk to it.
pkg_check_modules(HIDAPI REQUIRED IMPORTED_TARGET hidapi-hidraw)

# ---------------------------------------------------------------- library ---

add_library(egg STATIC
    src/egg/device.cpp
    src/egg/settings.cpp
)
target_include_directories(egg PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(egg PUBLIC PkgConfig::HIDAPI)
target_compile_options(egg PRIVATE -Wall -Wextra)

# -------------------------------------------------------------------- cli ---

add_executable(egg-cli src/cli/main.cpp)
target_link_libraries(egg-cli PRIVATE egg)
target_compile_options(egg-cli PRIVATE -Wall -Wextra)

# -------------------------------------------------------------------- gui ---

find_package(Qt6 6.2 COMPONENTS Widgets QUIET)
if(Qt6_FOUND)
    set(CMAKE_AUTOMOC ON)
    add_executable(egg-gui
        src/gui/main.cpp
        src/gui/MainWindow.cpp
    )
    target_link_libraries(egg-gui PRIVATE egg Qt6::Widgets)
    target_compile_options(egg-gui PRIVATE -Wall -Wextra)
    install(TARGETS egg-gui RUNTIME DESTINATION bin)
else()
    message(STATUS "Qt6 Widgets not found - building egg-cli only")
endif()

install(TARGETS egg-cli RUNTIME DESTINATION bin)
install(FILES udev/70-endgamegear.rules
        DESTINATION lib/udev/rules.d)
