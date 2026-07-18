# Add the source files in the "data" directory to the target.
target_sources(${MYLIB_CODE}
        PRIVATE
        cue_player.cpp
        cue_player_register.cpp
        mixer.cpp
        voice_pool.cpp
)