cmake_minimum_required(VERSION 3.28)

project(Breakout VERSION 0.1 LANGUAGES CXX C)
message(STATUS "CMAKE_PROJECT_VERSION: '${CMAKE_PROJECT_VERSION}'")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COLOR_DIAGNOSTICS ON)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_EXTENSIONS OFF)

message(STATUS "CMAKE_C_COMPILER_ID: '${CMAKE_C_COMPILER_ID}'")
message(STATUS "CMAKE_CXX_COMPILER_ID: '${CMAKE_CXX_COMPILER_ID}'")

set(CMAKE_CXX_STANDARD 23)

message(STATUS "CMAKE_CXX_STANDARD: '${CMAKE_CXX_STANDARD}'")

set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

include_directories(BEFORE "src")

if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    add_definitions("-D_CRT_SECURE_NO_WARNINGS")
    add_definitions("-DADT_STD_TYPES")
endif()

add_executable(
    ${CMAKE_PROJECT_NAME}
    src/main.cc
    src/gl/gl.cc
    src/controls.cc
    src/frame.cc
    src/Shader.cc
    src/json/Lexer.cc
    src/json/Parser.cc
    src/gltf/gltf.cc
    src/parser/Wave.cc
    src/parser/ttf.cc
    src/texture.cc
    src/Model.cc
    src/text.cc
    src/game.cc
    src/app.cc
    src/audio.cc
)

# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ftime-trace")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftime-trace")

add_definitions("-DADT_DEFER_LESS_TYPING")
add_definitions("-DADT_LOGS_LESS_TYPING")

if (CMAKE_BUILD_TYPE MATCHES "Release" AND CMAKE_SYSTEM_NAME MATCHES "Windows")
    set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY WIN32_EXECUTABLE TRUE)

    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto=thin")
    endif()
endif()

if (LOGS)
    add_definitions("-DADT_LOGS")
endif()

# debug logs defines
if (D_TTF)
    add_definitions("-DD_TTF")
endif()
if (D_GLTF)
    add_definitions("-DD_GLTF")
endif()
if (D_TEXTURE)
    add_definitions("-DD_TEXTURE")
endif()
if (D_WAVE)
    add_definitions("-DD_WAVE")
endif()
if (ADT_AVX2)
    add_definitions("-DADT_AVX2")
    set(ADT_SSE4_2 "1") # if we have avx2 there's sse4.1 either right?
endif()
if (ADT_SSE4_2)
    add_definitions("-DADT_SSE4_2")
endif()

if (ADT_SSE4_2)
    if(MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE4.2")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /arch:SSE4.2")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.2")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse4.2")
    endif()
endif()

if (ADT_AVX2)
    if(MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX2")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /arch:AVX2")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mavx2")
    endif()
endif()

if (CMAKE_BUILD_TYPE MATCHES "Asan")
    set(CMAKE_BUILD_TYPE "Debug")

    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
    else()

        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g3 -fsanitize=undefined -fsanitize=address")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g3 -fsanitize=undefined -fsanitize=address")

        if (CMAKE_SYSTEM_NAME MATCHES "Windows")
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -shared-libsan")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -shared-libsan")
            # doesn't work...
            # target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE clang_rt.asan_cxx-x86_64.lib clang_rt.asan-x86_64.lib)
        else()
            add_link_options(-fsanitize=undefined -fsanitize=address)
        endif()

    endif()
endif()

message(STATUS "CMAKE_BUILD_TYPE: '${CMAKE_BUILD_TYPE}'")

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /GR") # disable exceptions
    add_definitions("/D_HAS_EXCEPTIONS=0")

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4146 /wd4200 /wd4312")
    set(CMAKE_CXX_FLAGS "/utf-8")
    set(CMAKE_C_FLAGS "/utf-8")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-threadsafe-statics -fno-rtti -fno-exceptions -fno-unwind-tables")
    if (NOT CMAKE_BUILD_TYPE MATCHES "Debug")
        SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -nodefaultlibs -lc -lm")
    endif()

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -fms-extensions -Wno-missing-braces")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -fms-extensions -Wno-missing-braces")
endif()

if (CMAKE_BUILD_TYPE MATCHES "Debug")
    add_definitions("-DADT_LOGS")
    add_definitions("-DADT_DBG_MEMORY")

    target_sources(
        ${CMAKE_PROJECT_NAME}
        PRIVATE
        src/test.cc
    )

    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4267 /wd4101 /wd4200 /wd4244")
    else()
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wmissing-field-initializers -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wmissing-field-initializers -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable")
    endif()
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-class-memaccess -Wno-cast-function-type")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-cast-function-type-mismatch")
endif()

message(STATUS "CMAKE_SYSTEM_NAME: '${CMAKE_SYSTEM_NAME}'")
message(STATUS "CMAKE_SOURCE_DIR: '${CMAKE_SOURCE_DIR}'")

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LINUX_PKGS REQUIRED gl egl wayland-client wayland-egl wayland-cursor libpipewire-0.3)

    pkg_check_modules(X11_PKG x11)
    if (${X11_PKG_FOUND})
        add_definitions("-DX11_LIB")
        include_directories(BEFORE ${X11_PKG_INCLUDE_DIRS})
        target_link_libraries(
            ${CMAKE_PROJECT_NAME} PRIVATE
            ${X11_PKG_LIBRARIES}
        )

        target_sources(
            ${CMAKE_PROJECT_NAME} PRIVATE
            src/platform/x11/X11Window.cc
            src/platform/x11/input.cc
        )
    endif()

    include_directories(BEFORE ${LINUX_PKGS_INCLUDE_DIRS})
    target_link_libraries(
        ${CMAKE_PROJECT_NAME} PRIVATE
        ${LINUX_PKGS_LIBRARIES} 
    )

    set(WLP "./src/platform/wayland")
    set(WLPD "${WLP}/wayland-protocols")
    execute_process(
        COMMAND pkg-config wayland-protocols --variable=pkgdatadir
        OUTPUT_VARIABLE WAYLAND_PROTOCOLS_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    execute_process(
        COMMAND pkg-config --variable=wayland_scanner wayland-scanner
        OUTPUT_VARIABLE WAYLAND_SCANNER
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(XDG_SHELL "${WAYLAND_PROTOCOLS_DIR}/stable/xdg-shell/xdg-shell.xml")
    set(POINTER_CONSTRAINTS "${WAYLAND_PROTOCOLS_DIR}/unstable/pointer-constraints/pointer-constraints-unstable-v1.xml")
    set(RELATIVE_POINTER "${WAYLAND_PROTOCOLS_DIR}/unstable/relative-pointer/relative-pointer-unstable-v1.xml")
    set(TEARING "${WAYLAND_PROTOCOLS_DIR}/staging/tearing-control/tearing-control-v1.xml")

    message(STATUS "WLP: '${WLP}'")
    message(STATUS "WLPD: '${WLPD}'")
    message(STATUS "WAYLAND_PROTOCOLS_DIR: '${WAYLAND_PROTOCOLS_DIR}'")
    message(STATUS "WAYLAND_SCANNER: '${WAYLAND_SCANNER}'")
    message(STATUS "XDG_SHELL: '${XDG_SHELL}'")
    message(STATUS "POINTER_CONSTRAINTS: '${POINTER_CONSTRAINTS}'")
    message(STATUS "RELATIVE_POINTER: '${RELATIVE_POINTER}'")

    execute_process(COMMAND mkdir -p ${CMAKE_SOURCE_DIR}/${WLPD})
    execute_process(COMMAND ${WAYLAND_SCANNER} client-header ${RELATIVE_POINTER}    ${CMAKE_SOURCE_DIR}/${WLPD}/relative-pointer-unstable-v1.h)
    execute_process(COMMAND ${WAYLAND_SCANNER} private-code  ${RELATIVE_POINTER}    ${CMAKE_SOURCE_DIR}/${WLPD}/relative-pointer-unstable-v1.c)
    execute_process(COMMAND ${WAYLAND_SCANNER} client-header ${POINTER_CONSTRAINTS} ${CMAKE_SOURCE_DIR}/${WLPD}/pointer-constraints-unstable-v1.h)
    execute_process(COMMAND ${WAYLAND_SCANNER} private-code  ${POINTER_CONSTRAINTS} ${CMAKE_SOURCE_DIR}/${WLPD}/pointer-constraints-unstable-v1.c)
    execute_process(COMMAND ${WAYLAND_SCANNER} client-header ${XDG_SHELL}           ${CMAKE_SOURCE_DIR}/${WLPD}/xdg-shell.h)
    execute_process(COMMAND ${WAYLAND_SCANNER} private-code  ${XDG_SHELL}           ${CMAKE_SOURCE_DIR}/${WLPD}/xdg-shell.c)

    target_sources(
        ${CMAKE_PROJECT_NAME} PRIVATE
        ${WLP}/Client.cc
        ${WLP}/input.cc
        ${WLPD}/pointer-constraints-unstable-v1.c
        ${WLPD}/relative-pointer-unstable-v1.c
        ${WLPD}/xdg-shell.c

        src/platform/pipewire/Mixer.cc
    )
elseif (CMAKE_SYSTEM_NAME MATCHES "Windows")
    find_package(OpenGL REQUIRED)
    message(STATUS "OpenGL: '${OpenGL}'")

    target_link_libraries(
        ${CMAKE_PROJECT_NAME} PRIVATE
        ${OPENGL_gl_LIBRARY}
    )

    target_sources(
        ${CMAKE_PROJECT_NAME}
        PRIVATE
        src/platform/win32/glad.cc
        src/platform/win32/input.cc
        src/platform/win32/Win32Window.cc
        src/platform/win32/Mixer.cc
    )
endif()

# install(TARGETS ${CMAKE_PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
