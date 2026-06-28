# My library root
set(MYLIB_SRC
        dummy.cpp
)

set(MYLIB_PUBLIC_DIR
        wit/
)

set(MYLIB_INCLUDE_DIR
        src/
)

# Static library defined
add_library(${MYLIB_CODE}
        STATIC
        ${MYLIB_SRC}
)

target_include_directories(${MYLIB_CODE}
        INTERFACE
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
