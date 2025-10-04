#include <iostream>

#define MINIAUDIO_IMPLEMENTATION
#include "audio.hpp"

int play_audio()
{
    ma_result result;
    ma_engine engine;

    result = ma_engine_init(NULL, &engine);
    if (result != MA_SUCCESS) {
        std::cout << "Failed to initialize audio engine.\n";
        return -1;
    }

    ma_engine_play_sound(&engine, "../assets/take_me_away.mp3", NULL);

    std::cout << "Press Enter to quit...\n";
    getchar();

    ma_engine_uninit(&engine);
    return 0;
}