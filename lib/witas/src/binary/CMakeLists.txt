# Add the source files in the "binary" directory to the target.
target_sources(${MYLIB_CODE}
        PRIVATE
        binary_reader.cpp
        binary_writer.cpp
        riff_chunk_iterator.cpp
        uuid.cpp
)