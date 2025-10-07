#include "audio.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

int music::music_list()
{
    if (!fs::exists(directory_path) || !fs::is_directory(directory_path)) {
        std::cerr << "Error: Directory does not exist or is not a directory." << std::endl;
        return 1;
    }

    std::cout << "Files in directory: " << directory_path << std::endl;

    for (const auto& entry : fs::directory_iterator(directory_path)) {
        if (fs::is_regular_file(entry.status())) {
            std::cout << entry.path().filename().string() << std::endl;
        }
    }

    return 0;
}