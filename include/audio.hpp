#ifndef AUDIO_HPP
#define AUDIO_HPP

#include "miniaudio/miniaudio.h"
#include <memory>
#include <string>
#include <variant>

class music {
private:
    typedef struct music_lib_struct {
        std::string music;
        std::unique_ptr<music_lib_struct> Next;
        music_lib_struct* Prev;
    } music_lib_struct;

    std::string directory_path;
    std::unique_ptr<music_lib_struct> starting;

public:
    music()
        : directory_path("../assets") { };
    ~music() { };

    int music_list();
    int play_audio(bool loop);
};

#endif // AUDIO_HPP