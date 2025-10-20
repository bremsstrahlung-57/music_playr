#pragma once
#include "db.hpp"
#include "miniaudio/miniaudio.h"
#include <string>
#include <vector>

static float volume = 0.5f;
class Music {
private:
  ma_engine engine;
  ma_sound sound;
  Database music_db;

  float paused_time = 0.0f;

  bool is_playing = false;
  bool is_paused = false;

  std::vector<Track> all_track_vector;

public:
  Music();
  ~Music();

  void play(const std::string filepath, int track_id);
  void pause(int track_id);
  void stop();
  bool is_finished();
  void set_volume(float v);
  float get_volume() const;
};