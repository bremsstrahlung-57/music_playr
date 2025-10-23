#pragma once
#include "db.hpp"
#include "miniaudio/miniaudio.h"
#include <string>
#include <vector>

static float volume = 1.0f;

enum class PlaybackState { Stopped, Playing, Paused };
class Music {
private:
  ma_engine engine;
  ma_sound sound;
  Database music_db;

  PlaybackState state = PlaybackState::Stopped;

  float paused_time = 0.0f;

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
  float current_time() const;
  float max_time() const;
  void set_position(float seek_point_in_seconds);
  PlaybackState get_state() const { return state; }
};