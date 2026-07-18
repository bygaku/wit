# Add the source files in the "data" directory to the target.
target_sources(${MYLIB_CODE}
        PRIVATE
        category_store.cpp
        project_data.cpp
        cue_collection.cpp
)