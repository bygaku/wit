# My library root
file(GLOB SOURCES CONFIGURE_DEPENDS "*.cpp")
set(MYLIB_PUBLIC_DIR wit/)
set(MYLIB_INCLUDE_DIR src/)

# Static library defined
add_library(${MYLIB_CODE}
        STATIC
        ${SOURCES}
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

if(MSVC)
target_compile_options(${MYLIB_CODE}
        PRIVATE
        /W4
        /permissive-
)
endif()