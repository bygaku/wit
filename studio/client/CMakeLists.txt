# wit/lib/studio/client/CMakeLists.txt
# WitStudio (Qt Widgets).
find_package(Qt6 REQUIRED COMPONENTS
        Core
        Gui
        Widgets
)

set(STUDIO_CLIENT_SRC
        main.cpp
)

add_executable(${APPLICATION_CODE}
        WIN32
        ${STUDIO_CLIENT_SRC}
)

target_include_directories(${APPLICATION_CODE}
        PRIVATE
        ${CMAKE_SOURCE_DIR}/wit/src      # wit internal headers via studio_core types
)

target_link_libraries(${APPLICATION_CODE}
        PRIVATE
        ${STUDIO_CODE}
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
)

if(MSVC)
    target_compile_options(${APPLICATION_CODE}
            PRIVATE
            /W4
            /permissive-
    )
endif()
