cmake_minimum_required(VERSION 4.2)
set(CMAKE_CXX_STANDARD          17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)
set(PROJ_CODE  "wit")  # complete name: wit middleware
set(MYLIB_CODE "wit")  # complete name: wit audio system (Runtime)

project(${PROJ_CODE}
        VERSION     0.9.0
        LANGUAGES   CXX
        DESCRIPTION "Educational sound middleware for student game development"
)

enable_testing()
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX
            "${CMAKE_SOURCE_DIR}/dist/wit-sdk-${PROJECT_VERSION}"
            CACHE PATH "Distribution staging directory" FORCE)
endif()

# Fetch third-party data first
add_subdirectory(external)
# wit static library
add_subdirectory(wit)

option(WIT_BUILD_STUDIO "Build WitStudio" OFF)  # CMake Configure failure on GitHub Actions.
# wit studio
if(WIT_BUILD_STUDIO)
    add_subdirectory(studio)
endif()