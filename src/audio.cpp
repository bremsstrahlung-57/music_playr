#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>

#define MINIAUDIO_IMPLEMENTATION
#include "audio.hpp"

int music::play_audio(bool loop)
{
    ma_result result;
    ma_engine engine;

    result = ma_engine_init(NULL, &engine);
    if (result != MA_SUCCESS) {
        std::cout << "Failed to initialize audio engine.\n";
        return -1;
    }

    if (!std::filesystem::exists(directory_path) || !std::filesystem::is_directory(directory_path)) {
        std::cerr << "Error: Directory not found or is not a directory: " << directory_path << std::endl;
        return 1;
    }

    for (const auto& entry : std::filesystem::directory_iterator(directory_path)) {
        if (std::filesystem::is_regular_file(entry.path())) {
            std::string extension = entry.path().extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
            if (extension == ".mp3" || extension == ".wav" || extension == ".flac" || extension == ".ogg") {
                ma_engine_play_sound(&engine, entry.path().c_str(), NULL);
                std::cout << "Press Enter to quit...\n";
                getchar();
            }
        }
    }

    ma_engine_uninit(&engine);
    return 0;
}