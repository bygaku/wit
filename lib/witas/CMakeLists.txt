# My library root
set(MYLIB_PUBLIC_DIR wit/)
set(MYLIB_INCLUDE_DIR src/)

# Static library defined
add_library(${MYLIB_CODE}
        STATIC
        ${SOURCES}
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
)
endif()

# Additional directories
add_subdirectory(src/binary)
add_subdirectory(src/util)