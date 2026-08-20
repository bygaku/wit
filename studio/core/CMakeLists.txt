# wit/lib/studio/core/CMakeLists.txt
# studio_core: authoring logic
find_package(Qt6 REQUIRED COMPONENTS
        Core
)

set(STUDIO_CORE_SRC
        project_builder.cpp
        audio_converter.cpp
        project_validator.cpp
        project_serializer.cpp
)

add_library(${STUDIO_CODE}
        STATIC
        ${STUDIO_CORE_SRC}
)

target_include_directories(${STUDIO_CODE}
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE
        ${CMAKE_SOURCE_DIR}/wit/src      # wit internal headers (binary/, data/)
)

target_link_libraries(${STUDIO_CODE}
        PUBLIC
        wit
        PRIVATE
        miniaudio   # decode
        Qt6::Core
)

if(MSVC)
    target_compile_options(${STUDIO_CODE}
            PRIVATE
            /W4
            /permissive-
            /utf-8
    )
endif()

# Headless contract tests: builder output is loaded back through the actual
# runtime parsers (ProjectData / Wccb). Run with ctest or directly.
if(WIT_STUDIO_BUILD_TESTS)
    enable_testing()

    add_executable(studio_test_roundtrip  test_roundtrip.cpp)
    add_executable(studio_test_serializer test_serializer.cpp)

    foreach(test_target studio_test_roundtrip studio_test_serializer)
        target_include_directories(${test_target} PRIVATE ${CMAKE_SOURCE_DIR}/wit/src)
        target_link_libraries(${test_target} PRIVATE ${STUDIO_CODE} Qt6::Core)
    endforeach()

    add_test(NAME studio_roundtrip  COMMAND studio_test_roundtrip)
    add_test(NAME studio_serializer COMMAND studio_test_serializer)
endif()