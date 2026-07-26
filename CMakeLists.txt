cmake_minimum_required(VERSION 4.2)
set(CMAKE_CXX_STANDARD          17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)
set(PROJ_CODE  "wit")  # complete name: wit middleware
set(MYLIB_CODE "wit")  # complete name: wit audio system (Runtime Library)

project(${PROJ_CODE}
        VERSION     1.0.0
        LANGUAGES   CXX
        DESCRIPTION "Educational sound middleware for student game development"
)

set(WIT_VERSION_PRERELEASE "beta.2") # HACK: リリース時削除
if(WIT_VERSION_PRERELEASE)
    set(WIT_VERSION_FULL "${PROJECT_VERSION}-${WIT_VERSION_PRERELEASE}")
else()
    set(WIT_VERSION_FULL "${PROJECT_VERSION}")
endif()

option(WIT_BUILD_STUDIO "Build WitStudio" OFF)  # CMake Configure failure on GitHub Actions.
option(WIT_STUDIO_BUILD_TESTS "Build the studio_core roundtrip tests" OFF)

if(MSVC)
    if(WIT_BUILD_STUDIO)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    else()
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
endif()

enable_testing()
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX
            "${CMAKE_SOURCE_DIR}/dist/wit-sdk-${WIT_VERSION_FULL}"
            CACHE PATH "Distribution staging directory" FORCE)
endif()

# Fetch third-party data first
add_subdirectory(external)
# wit static library
add_subdirectory(wit)
# wit studio
if(WIT_BUILD_STUDIO)
    add_subdirectory(studio)
endif()

if(MSVC)
    set(CPACK_GENERATOR "ZIP")
    if(WIT_BUILD_STUDIO)
        set(CPACK_PACKAGE_FILE_NAME "WitStudio-${WIT_VERSION_FULL}-x64")
    else()
        set(CPACK_PACKAGE_FILE_NAME "Witas-${WIT_VERSION_FULL}-dxlib-x64")
    endif()
    include(CPack)
endif()