cmake_minimum_required(VERSION 3.21)

project(PostLive
    VERSION 1.0.0.2
    DESCRIPTION "A learning-focused live streaming desktop client"
    HOMEPAGE_URL "https://github.com/ohto-ai/PostLive"
    LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR
        "PostLive currently targets Windows only (DirectShow capture, DbgHelp minidumps).")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 5.15 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Network
    Multimedia
    MultimediaWidgets)

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------

set(POSTLIVE_SOURCES
    src/main.cpp

    src/app/LiveLoginDialog.cpp
    src/app/LiveLoginSettingDialog.cpp
    src/app/LivePlatform.cpp

    src/core/AsyncDownloader.cpp
    src/core/WidgetConfigInfo.cpp
    src/core/utils.cpp

    src/widgets/BaseDialog.cpp
    src/widgets/BaseMainWindow.cpp
    src/widgets/BaseWidget.cpp
    src/widgets/DragableDialog.cpp
    src/widgets/DragableMainWindow.cpp
    src/widgets/DragableWidget.cpp
    src/widgets/PasswordEdit.cpp
    src/widgets/ShadowWidget.cpp
    src/widgets/TouchedLabel.cpp
    src/widgets/TouchedLineEdit.cpp)

set(POSTLIVE_HEADERS
    src/app/LiveLoginDialog.h
    src/app/LiveLoginSettingDialog.h
    src/app/LivePlatform.h

    src/core/AsyncDownloader.h
    src/core/GeneralSignal.h
    src/core/WidgetConfigInfo.h
    src/core/utils.h

    src/widgets/BaseDialog.h
    src/widgets/BaseMainWindow.h
    src/widgets/BaseWidget.h
    src/widgets/BaseWidgetInterface.h
    src/widgets/DragableDialog.h
    src/widgets/DragableMainWindow.h
    src/widgets/DragableWidget.h
    src/widgets/PasswordEdit.h
    src/widgets/ShadowWidget.h
    src/widgets/TouchedLabel.h
    src/widgets/TouchedLineEdit.h)

set(POSTLIVE_FORMS
    src/app/LiveLoginDialog.ui
    src/app/LiveLoginSettingDialog.ui
    src/app/LivePlatform.ui)

set(POSTLIVE_RESOURCES
    src/resources/LiveLoginDialog.qrc)

# Version/icon resource is generated so that the version stays in sync with
# project(). The icon path is absolute, which avoids rc.exe path ambiguity.
set(POSTLIVE_ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/src/resources/res/live.ico")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/PostLive.rc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/PostLive.rc"
    @ONLY)

add_executable(PostLive WIN32
    ${POSTLIVE_SOURCES}
    ${POSTLIVE_HEADERS}
    ${POSTLIVE_FORMS}
    ${POSTLIVE_RESOURCES}
    "${CMAKE_CURRENT_BINARY_DIR}/PostLive.rc")

target_include_directories(PostLive PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/app"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/core"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/widgets"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party")

target_compile_definitions(PostLive PRIVATE
    POSTLIVE_VERSION="${PROJECT_VERSION}"
    UNICODE
    _UNICODE)

if(MSVC)
    target_compile_options(PostLive PRIVATE
        "$<$<COMPILE_LANGUAGE:CXX>:/utf-8>")
endif()

target_link_libraries(PostLive PRIVATE
    Qt5::Widgets
    Qt5::Network
    Qt5::Multimedia
    Qt5::MultimediaWidgets
    ws2_32
    dbghelp)

# Local development server for the authentication API.
add_executable(PostLiveMockServer tools/mock-server/main.cpp)
target_include_directories(PostLiveMockServer PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party")
target_link_libraries(PostLiveMockServer PRIVATE
    Qt5::Core
    ws2_32)
if(MSVC)
    target_compile_options(PostLiveMockServer PRIVATE
        "$<$<COMPILE_LANGUAGE:CXX>:/utf-8>")
endif()
set_target_properties(PostLiveMockServer PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tools/mock-server")

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/tools" PREFIX "Tools"
    FILES tools/mock-server/main.cpp)

# ---------------------------------------------------------------------------
# Runtime deployment
# ---------------------------------------------------------------------------

set(POSTLIVE_FFMPEG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/ffmpeg"
    CACHE PATH "Directory containing a standalone FFmpeg build (ffmpeg.exe)")

find_program(POSTLIVE_FFMPEG_EXECUTABLE
    NAMES ffmpeg.exe
    HINTS
        "${POSTLIVE_FFMPEG_DIR}/bin"
        "${POSTLIVE_FFMPEG_DIR}"
    NO_DEFAULT_PATH
    NO_CACHE)

if(NOT POSTLIVE_FFMPEG_EXECUTABLE)
    find_program(POSTLIVE_FFMPEG_EXECUTABLE NAMES ffmpeg.exe NO_CACHE)
endif()

if(POSTLIVE_FFMPEG_EXECUTABLE)
    get_filename_component(POSTLIVE_FFMPEG_BIN_DIR
        "${POSTLIVE_FFMPEG_EXECUTABLE}" DIRECTORY)
    message(STATUS "FFmpeg runtime: ${POSTLIVE_FFMPEG_BIN_DIR}")
else()
    set(POSTLIVE_FFMPEG_BIN_DIR "")
    message(STATUS
        "FFmpeg runtime not found. The app will build, but streaming needs "
        "ffmpeg.exe. Set POSTLIVE_FFMPEG_DIR or place a build under "
        "third_party/ffmpeg/.")
endif()

add_custom_command(TARGET PostLive POST_BUILD
    COMMAND "${CMAKE_COMMAND}"
        "-DRUNTIME_DIR=$<TARGET_FILE_DIR:PostLive>"
        "-DASSETS_DIR=${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "-DCONFIG_DIR=${CMAKE_CURRENT_SOURCE_DIR}/config"
        "-DFFMPEG_BIN_DIR=${POSTLIVE_FFMPEG_BIN_DIR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/DeployRuntime.cmake"
    COMMENT "Deploying runtime files next to PostLive.exe"
    VERBATIM)

get_target_property(_qmake_executable Qt5::qmake IMPORTED_LOCATION)
get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)

find_program(WINDEPLOYQT_EXECUTABLE
    NAMES windeployqt
    HINTS "${_qt_bin_dir}"
    NO_DEFAULT_PATH)

if(WINDEPLOYQT_EXECUTABLE)
    add_custom_target(deploy-qt
        COMMAND "${CMAKE_COMMAND}"
            "-DWINDEPLOYQT=${WINDEPLOYQT_EXECUTABLE}"
            "-DQT_BIN_DIR=${_qt_bin_dir}"
            "-DTARGET_EXE=$<TARGET_FILE:PostLive>"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/DeployQt.cmake"
        DEPENDS PostLive
        COMMENT "Deploying Qt runtime next to PostLive.exe"
        VERBATIM)
endif()

# ---------------------------------------------------------------------------
# IDE / install
# ---------------------------------------------------------------------------

set_target_properties(PostLive PROPERTIES
    OUTPUT_NAME "PostLive"
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:PostLive>"
    VS_DEBUGGER_ENVIRONMENT "PATH=${_qt_bin_dir}\;%PATH%")

set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    PROPERTY VS_STARTUP_PROJECT PostLive)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "Source Files"
    FILES
        ${POSTLIVE_SOURCES}
        ${POSTLIVE_HEADERS}
        ${POSTLIVE_FORMS}
        ${POSTLIVE_RESOURCES})

include(GNUInstallDirs)
install(TARGETS PostLive RUNTIME DESTINATION .)
install(DIRECTORY assets/ DESTINATION assets)
install(FILES config/application.default.json
    DESTINATION . RENAME application.json)
install(FILES config/users.default.json
    DESTINATION . RENAME users.json)

message(STATUS "PostLive ${PROJECT_VERSION}")
message(STATUS "  Qt: ${Qt5_DIR}")
message(STATUS "  Generator: ${CMAKE_GENERATOR}")
if(CMAKE_BUILD_TYPE)
    message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
endif()
