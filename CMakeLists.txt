cmake_minimum_required(VERSION 4.2)
set(CMAKE_CXX_STANDARD          17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)
set(PROJ_CODE "wit")    # complete name: wit audio system
set(MYLIB_CODE  "wit")  # complete name: wit audio system

project(${PROJ_CODE}
        VERSION     0.1
        LANGUAGES   CXX
        DESCRIPTION "Educational sound middleware for student game development"
)

enable_testing()

# wit static library
add_subdirectory(lib)
# test directory
add_subdirectory(test)