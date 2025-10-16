#ifndef AUDIO_HPP
#define AUDIO_HPP

#include "db.hpp"
#include "miniaudio/miniaudio.h"
#include <cstddef>
#include <iostream>
#include <vector>

enum class PlayState { NEXT, PREV, QUIT, CONTINUE };

class Music {
private:
  ma_engine engine;
  int SONG_ID;
  Database _MUSIC_DB;
  std::vector<Track> all_track_vector = _MUSIC_DB.get_all_tracks();

public:
  Music() {
    if (ma_engine_init(NULL, &engine) != MA_SUCCESS)
      std::cout << "Failed to init engine\n";
  };
  ~Music() { ma_engine_uninit(&engine); };
  void music_menu();
  PlayState play_audio(const char *filepath);
};

#endif // AUDIO_HPP