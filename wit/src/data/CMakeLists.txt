# Add the source files in the "data" directory to the target.
target_sources(${MYLIB_CODE}
        PRIVATE
        project_data.cpp
        category_store.cpp
        cue_collection.cpp
        cue_collection_store.cpp
        memory_waveform_provider.cpp
        streaming_waveform_provider.cpp
        waveform_provider_store.cpp
        streaming_reader.cpp
)