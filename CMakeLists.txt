cmake_minimum_required(VERSION 3.24)

set(SKYLANDERS_BUILD_BUNDLED_CEMU_DEFAULT OFF)
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/cemu/CMakeLists.txt")
    set(SKYLANDERS_BUILD_BUNDLED_CEMU_DEFAULT ON)
endif()

option(SKYLANDERS_BUILD_BUNDLED_CEMU "Build and bundle the external Cemu executable from the submodule" ${SKYLANDERS_BUILD_BUNDLED_CEMU_DEFAULT})
option(SKYLANDERS_EMBED_CEMU "Build SkylandersEngine against the embedded Cemu submodule" OFF)

if ((SKYLANDERS_EMBED_CEMU OR SKYLANDERS_BUILD_BUNDLED_CEMU)
    AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/cemu/dependencies/vcpkg/scripts/buildsystems/vcpkg.cmake"
    AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE
        "${CMAKE_CURRENT_SOURCE_DIR}/extern/cemu/dependencies/vcpkg/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
    set(CMAKE_FIND_PACKAGE_PREFER_CONFIG TRUE CACHE BOOL "" FORCE)
    set(VCPKG_MANIFEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extern/cemu" CACHE PATH "" FORCE)
    set(VCPKG_INSTALLED_DIR "${CMAKE_CURRENT_BINARY_DIR}/vcpkg_installed" CACHE PATH "" FORCE)
    set(VCPKG_TARGET_TRIPLET "x64-windows-static" CACHE STRING "" FORCE)
endif()

project(SkylandersEngine LANGUAGES CXX)

enable_testing()

add_library(skylanders_launcher
    src/CharacterCatalog.cpp
    src/CemuLocator.cpp
    src/CemuConfigEditors.cpp
    src/EmulatorCore.cpp
    src/FigureStore.cpp
    src/GameCatalog.cpp
    src/GraphicPackManager.cpp
    src/InputBridge.cpp
    src/PortalBridge.cpp
    src/ProcessManager.cpp
    src/RomLocator.cpp
)

target_include_directories(skylanders_launcher
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(skylanders_launcher PUBLIC cxx_std_20)

if (WIN32)
    target_compile_definitions(skylanders_launcher PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_link_libraries(skylanders_launcher PUBLIC Comdlg32 Urlmon)
endif()

if ((SKYLANDERS_EMBED_CEMU OR SKYLANDERS_BUILD_BUNDLED_CEMU) AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/cemu/CMakeLists.txt")
    set(ENABLE_VCPKG ON CACHE BOOL "" FORCE)
    set(ENABLE_VULKAN ON CACHE BOOL "" FORCE)
    set(ENABLE_OPENGL ON CACHE BOOL "" FORCE)
    set(ENABLE_WXWIDGETS ON CACHE BOOL "" FORCE)
    set(ENABLE_DISCORD_RPC OFF CACHE BOOL "" FORCE)
    set(ENABLE_LIBUSB ON CACHE BOOL "" FORCE)
    if (WIN32)
        add_compile_definitions(UNICODE _UNICODE)
    endif()

    add_subdirectory(extern/cemu EXCLUDE_FROM_ALL)
endif()

if (SKYLANDERS_EMBED_CEMU AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/cemu/CMakeLists.txt")
    target_compile_definitions(skylanders_launcher PUBLIC SKYLANDERS_HAS_CEMU=1)
    target_include_directories(skylanders_launcher PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/extern/cemu/src"
    )

    foreach(cemu_target IN ITEMS CemuCafe CemuCommon CemuConfig CemuInput CemuAudio CemuUtil CemuComponents)
        if (TARGET ${cemu_target})
            target_link_libraries(skylanders_launcher PUBLIC ${cemu_target})
        endif()
    endforeach()
else()
    target_compile_definitions(skylanders_launcher PUBLIC SKYLANDERS_HAS_CEMU=0)
endif()

add_executable(SkylandersEngine WIN32
    src/main.cpp
)

target_link_libraries(SkylandersEngine PRIVATE skylanders_launcher)
target_compile_features(SkylandersEngine PRIVATE cxx_std_20)

add_executable(SkylandersEngineTests
    tests/RomLocatorTests.cpp
)

target_link_libraries(SkylandersEngineTests PRIVATE skylanders_launcher)
target_compile_features(SkylandersEngineTests PRIVATE cxx_std_20)

add_test(NAME SkylandersEngineTests COMMAND SkylandersEngineTests)

set(ULTRALIGHT_SDK_ROOT "" CACHE PATH "Path to the Ultralight SDK root")
if (NOT ULTRALIGHT_SDK_ROOT
    AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/sdks/ultralight/include/Ultralight/Ultralight.h")
    set(ULTRALIGHT_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/sdks/ultralight" CACHE PATH "Path to the Ultralight SDK root" FORCE)
endif()

if (ULTRALIGHT_SDK_ROOT)
    set(ULTRALIGHT_INCLUDE_DIR "${ULTRALIGHT_SDK_ROOT}/include")
    set(ULTRALIGHT_LIBRARY_DIR "${ULTRALIGHT_SDK_ROOT}/lib")
    set(ULTRALIGHT_BINARY_DIR "${ULTRALIGHT_SDK_ROOT}/bin")

    if (EXISTS "${ULTRALIGHT_INCLUDE_DIR}/Ultralight/Ultralight.h")
        target_compile_definitions(SkylandersEngine PRIVATE SKYLANDERS_WITH_ULTRALIGHT=1)
        target_include_directories(SkylandersEngine PRIVATE "${ULTRALIGHT_INCLUDE_DIR}")
        target_link_directories(SkylandersEngine PRIVATE "${ULTRALIGHT_LIBRARY_DIR}")
        target_link_libraries(SkylandersEngine PRIVATE UltralightCore AppCore Ultralight WebCore)

        if (WIN32 AND EXISTS "${ULTRALIGHT_BINARY_DIR}")
            add_custom_command(TARGET SkylandersEngine POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${ULTRALIGHT_BINARY_DIR}"
                    "$<TARGET_FILE_DIR:SkylandersEngine>"
            )
        endif()

        if (EXISTS "${ULTRALIGHT_SDK_ROOT}/resources")
            add_custom_command(TARGET SkylandersEngine POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${ULTRALIGHT_SDK_ROOT}/resources"
                    "$<TARGET_FILE_DIR:SkylandersEngine>/assets/resources"
            )
        endif()

        if (EXISTS "${ULTRALIGHT_SDK_ROOT}/inspector")
            add_custom_command(TARGET SkylandersEngine POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${ULTRALIGHT_SDK_ROOT}/inspector"
                    "$<TARGET_FILE_DIR:SkylandersEngine>/assets/inspector"
            )
        endif()
    else()
        message(WARNING "ULTRALIGHT_SDK_ROOT was set, but Ultralight/Ultralight.h was not found. Building fallback executable.")
        target_compile_definitions(SkylandersEngine PRIVATE SKYLANDERS_WITH_ULTRALIGHT=0)
    endif()
else()
    message(WARNING "ULTRALIGHT_SDK_ROOT is not set. Building fallback executable.")
    target_compile_definitions(SkylandersEngine PRIVATE SKYLANDERS_WITH_ULTRALIGHT=0)
endif()

add_custom_command(TARGET SkylandersEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:SkylandersEngine>/assets"
)

if (SKYLANDERS_BUILD_BUNDLED_CEMU AND TARGET CemuBin)
    add_dependencies(SkylandersEngine CemuBin)
    add_custom_command(TARGET SkylandersEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:SkylandersEngine>/cemu"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:CemuBin>"
            "$<TARGET_FILE_DIR:SkylandersEngine>/cemu/Cemu.exe"
    )

    foreach(cemu_runtime_dir IN ITEMS resources gameProfiles)
        if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/cemu/bin/${cemu_runtime_dir}")
            add_custom_command(TARGET SkylandersEngine POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${CMAKE_CURRENT_SOURCE_DIR}/extern/cemu/bin/${cemu_runtime_dir}"
                    "$<TARGET_FILE_DIR:SkylandersEngine>/cemu/${cemu_runtime_dir}"
            )
        endif()
    endforeach()
endif()

foreach(skylanders_stub IN ITEMS
    SpyrosAdventure.exe
    Giants.exe
    SwapForce.exe
    TrapTeam.exe
    SuperChargers.exe
    Imaginators.exe)
    add_custom_command(TARGET SkylandersEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:SkylandersEngine>"
            "$<TARGET_FILE_DIR:SkylandersEngine>/${skylanders_stub}"
    )
endforeach()
