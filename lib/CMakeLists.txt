# witas/lib/CMakeLists.txt

# Fetch third-party data first
add_subdirectory(external)

# Wit library defined
set(MYLIB_DIR
        wit/
)

set(MYLIB_SRC
        ${MYLIB_DIR}/dummy.cpp
)

set(MYLIB_INCLUDE_DIR
        ${MYLIB_DIR}/
)

# Static library defined
add_library(${MYLIB_CODE}
        STATIC
        ${MYLIB_SRC}
)

target_include_directories(${MYLIB_CODE}
        PRIVATE
        ${MYLIB_INCLUDE_DIR}
)

target_include_directories(${MYLIB_CODE}
        INTERFACE
        ${MYLIB_INCLUDE_DIR}
)

target_link_libraries(${MYLIB_CODE}
        PRIVATE
        miniaudio
)
