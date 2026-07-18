# My library root
set(MYLIB_PUBLIC_DIR wit/)
set(MYLIB_INCLUDE_DIR src/)

# Static library defined
add_library(${MYLIB_CODE}
        STATIC
        src/audio_engine.cpp
        src/wit_c_api.cpp
        src/miniaudio_impl.cpp
)

target_include_directories(${MYLIB_CODE}
        PUBLIC
        ${MYLIB_PUBLIC_DIR}
)

target_include_directories(${MYLIB_CODE}
        PRIVATE
        ${MYLIB_INCLUDE_DIR}
)

target_link_libraries(${MYLIB_CODE}
        PRIVATE
        miniaudio
)

if(MSVC)
target_compile_options(${MYLIB_CODE}
        PRIVATE
        /W4
        /permissive-
        /utf-8
)
endif()

# Additional directories
add_subdirectory(src/binary)
add_subdirectory(src/util)
add_subdirectory(src/data)
add_subdirectory(src/runtime)
