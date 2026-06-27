# wit/lib/external/CMakeLists.txt
# about this CMakeLists: Fetch third-party contents
include(FetchContent)

# Fetch miniaudio v0.11.25
FetchContent_Declare(
        miniaudio
        GIT_REPOSITORY https://github.com/mackron/miniaudio.git
        GIT_TAG        0.11.25
        GIT_SHALLOW    TRUE
)

# Suppressing unwanted targets
set(MINIAUDIO_NO_EXTRA_NODES ON  CACHE BOOL "" FORCE)
set(MINIAUDIO_NO_LIBVORBIS   ON  CACHE BOOL "" FORCE)
set(MINIAUDIO_NO_LIBOPUS     ON  CACHE BOOL "" FORCE)

set(MINIAUDIO_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(MINIAUDIO_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(MINIAUDIO_BUILD_TOOLS    OFF CACHE BOOL "" FORCE)

# From the wit library side, reference it using `target_link_libraries(wit PRIVATE miniaudio)`
FetchContent_MakeAvailable(miniaudio)
