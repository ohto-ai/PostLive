find_package(Qt5 5.15 REQUIRED COMPONENTS Test)

add_executable(PostLiveTests
    test_core.cpp
    # Header-only Q_OBJECT classes must be listed for AUTOMOC to pick them up.
    ../src/core/GeneralSignal.h
    ../src/core/AsyncDownloader.cpp
    ../src/core/Logging.cpp
    ../src/core/WidgetConfigInfo.cpp
    ../src/core/utils.cpp
    testdata.qrc)

target_include_directories(PostLiveTests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/core"
    "${CMAKE_CURRENT_SOURCE_DIR}/../third_party")

target_compile_definitions(PostLiveTests PRIVATE
    UNICODE
    _UNICODE)

if(MSVC)
    target_compile_options(PostLiveTests PRIVATE
        "$<$<COMPILE_LANGUAGE:CXX>:/utf-8>")
endif()

target_link_libraries(PostLiveTests PRIVATE
    Qt5::Test
    Qt5::Network
    Qt5::Widgets
    crypt32)

add_test(NAME core COMMAND PostLiveTests)
set_tests_properties(core PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Make the Qt DLLs next to the Qt bin dir resolvable no matter how ctest is
# invoked (ENVIRONMENT_MODIFICATION needs CMake >= 3.22; on older CMake and on
# CI, Qt's bin dir is expected to be on PATH already).
get_target_property(_qt_test_location Qt5::Test IMPORTED_LOCATION_RELEASE)
if(NOT _qt_test_location)
    get_target_property(_qt_test_location Qt5::Test IMPORTED_LOCATION_DEBUG)
endif()
if(NOT _qt_test_location)
    get_target_property(_qt_test_location Qt5::Test IMPORTED_LOCATION)
endif()
if(_qt_test_location)
    get_filename_component(_qt_bin_dir "${_qt_test_location}" DIRECTORY)
    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.22)
        set_property(TEST core APPEND PROPERTY ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:${_qt_bin_dir}")
    endif()
endif()
