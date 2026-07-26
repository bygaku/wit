# wit/lib/studio/client/CMakeLists.txt
# WitStudio (Qt Widgets).
find_package(Qt6 REQUIRED COMPONENTS
        Core
        Gui
        Widgets
)

set(STUDIO_CLIENT_SRC
        main.cpp
        main_window.cpp
        cue_properties_editor_widget.cpp
        project_properties_editor_widget.cpp
        object_tree_widget.cpp
        cue_collection_properties_editor_widget.cpp
        category_editor_dialog.cpp
        issue_list_widget.cpp
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
            /utf-8
    )
endif()

if(WIN32)
    get_target_property(_qt_qmake Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qt_qmake}" DIRECTORY)
    find_program(WINDEPLOYQT_EXE windeployqt HINTS "${_qt_bin_dir}")

    if(WINDEPLOYQT_EXE)
        add_custom_command(TARGET ${APPLICATION_CODE} POST_BUILD
                COMMAND "${WINDEPLOYQT_EXE}"
                --no-translations
                --$<IF:$<CONFIG:Debug>,debug,release>
                "$<TARGET_FILE:${APPLICATION_CODE}>"
                COMMENT "Deploying Qt runtime next to ${APPLICATION_CODE}"
                VERBATIM)
    else()
        message(WARNING
                "windeployqt not found; ${APPLICATION_CODE} will not run "
                "on machines without Qt installed.")
    endif()
endif()

install(TARGETS ${APPLICATION_CODE}
        RUNTIME DESTINATION studio
        CONFIGURATIONS Release)

if(WIN32 AND WINDEPLOYQT_EXE)
    install(CODE "execute_process(COMMAND \"${WINDEPLOYQT_EXE}\"
            --no-translations
            --release
            --dir \"\${CMAKE_INSTALL_PREFIX}/studio\"
            \"\${CMAKE_INSTALL_PREFIX}/studio/${APPLICATION_CODE}.exe\")"
            CONFIGURATIONS Release)
endif()