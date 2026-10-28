# Copy the runtime files that PostLive expects next to its executable.
#
# Invoked from a POST_BUILD command:
#   cmake -DRUNTIME_DIR=... -DASSETS_DIR=... -DCONFIG_DIR=... -DFFMPEG_BIN_DIR=... -P DeployRuntime.cmake
#
# application.json and users.json are only created when missing so that a
# developer's local state is never overwritten by a rebuild.

if(NOT DEFINED RUNTIME_DIR)
    message(FATAL_ERROR "RUNTIME_DIR is required")
endif()

file(MAKE_DIRECTORY
    "${RUNTIME_DIR}/assets"
    "${RUNTIME_DIR}/user/avatar")

if(EXISTS "${ASSETS_DIR}/pilipili-logo.png")
    file(COPY_FILE
        "${ASSETS_DIR}/pilipili-logo.png"
        "${RUNTIME_DIR}/assets/pilipili-logo.png"
        ONLY_IF_DIFFERENT)
endif()

foreach(_config application users)
    if(EXISTS "${CONFIG_DIR}/${_config}.default.json"
       AND NOT EXISTS "${RUNTIME_DIR}/${_config}.json")
        file(COPY_FILE
            "${CONFIG_DIR}/${_config}.default.json"
            "${RUNTIME_DIR}/${_config}.json")
    endif()
endforeach()

if(DEFINED FFMPEG_BIN_DIR AND NOT FFMPEG_BIN_DIR STREQUAL "")
    file(GLOB _ffmpeg_runtime
        "${FFMPEG_BIN_DIR}/ffmpeg.exe"
        "${FFMPEG_BIN_DIR}/av*.dll"
        "${FFMPEG_BIN_DIR}/sw*.dll")

    foreach(_file IN LISTS _ffmpeg_runtime)
        get_filename_component(_name "${_file}" NAME)
        file(COPY_FILE
            "${_file}"
            "${RUNTIME_DIR}/${_name}"
            ONLY_IF_DIFFERENT)
    endforeach()
endif()
