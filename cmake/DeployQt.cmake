# Run the windeployqt that belongs to the Qt 5 kit used for the build.
#
# windeployqt locates its dependencies through PATH, so a Qt 6 installation
# earlier in PATH can make it look in the wrong bin directory. Prepend the
# build's Qt bin directory before invoking it.
#
#   cmake -DWINDEPLOYQT=... -DQT_BIN_DIR=... -DTARGET_EXE=... -P DeployQt.cmake

if(NOT DEFINED WINDEPLOYQT OR NOT DEFINED QT_BIN_DIR OR NOT DEFINED TARGET_EXE)
    message(FATAL_ERROR "WINDEPLOYQT, QT_BIN_DIR and TARGET_EXE are required")
endif()

set(ENV{PATH} "${QT_BIN_DIR};$ENV{PATH}")

execute_process(
    COMMAND "${WINDEPLOYQT}"
        --no-translations
        --no-opengl-sw
        "${TARGET_EXE}"
    RESULT_VARIABLE _result)

if(NOT _result EQUAL 0)
    message(FATAL_ERROR "windeployqt failed with exit code ${_result}")
endif()
