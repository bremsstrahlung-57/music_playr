#ifndef AUDIO_HPP
#define AUDIO_HPP

#include <memory>
#include <string>
#include <variant>

#include "miniaudio/miniaudio.h"

class music {
   private:
    std::string directory_path;

   public:
    music() : directory_path("../assets") {};
    ~music() {};

    int music_list();
    int play_audio(bool loop);
};

#endif  // AUDIO_HPP