#include "miniaudio/miniaudio.h"
#include <cstdio>
#include <iostream>
#include <ostream>
#include <string>
#include <unistd.h>

#define MINIAUDIO_IMPLEMENTATION
#include "audio.hpp"

Music::Music() {
  if (ma_engine_init(NULL, &engine) != MA_SUCCESS) {
    std::cerr << "Failed to init engine\n";
  }
  all_track_vector = music_db.get_all_tracks();
};

Music::~Music() {
  ma_sound_uninit(&sound);
  ma_engine_uninit(&engine);
};

void Music::play(const std::string filepath, int track_id) {
  if (is_paused) {
    ma_sound_start(&sound);
    is_playing = true;
    is_paused = false;
    return;
  }

  if (is_playing) {
    pause(track_id);
    return;
  }

  ma_result result =
      ma_sound_init_from_file(&engine, filepath.c_str(), 0, NULL, NULL, &sound);

  if (result != MA_SUCCESS) {
    std::cerr << "Failed to load sound: " << filepath << std::endl;
    return;
  }

  ma_sound_seek_to_second(&sound, music_db.last_played_timestamp(track_id));
  ma_sound_start(&sound);
  music_db.increase_play_count(track_id);
  is_playing = true;
  is_paused = false;
}

void Music::pause(int track_id) {
  if (ma_sound_is_playing(&sound)) {
    ma_sound_get_cursor_in_seconds(&sound, &paused_time);
    music_db.add_last_played_timestamp(track_id, paused_time);
    ma_sound_stop(&sound);
    is_paused = true;
    is_playing = false;
  }
  return;
}

void Music::stop() {
  if (is_playing || is_paused) {
    ma_sound_uninit(&sound);
    is_playing = false;
    is_paused = false;
  }
}

bool Music::is_finished() { return is_playing && !ma_sound_is_playing(&sound); }

void Music::set_volume(float v) {
  if (v < 0.0f)
    v = 0.0f;
  else if (v > 1.0f)
    v = 1.0f;

  volume = v;

  ma_sound_set_volume(&sound, volume);
}

float Music::get_volume() const { return volume; }

float Music::current_time() const {
  float curr_time = 0.0f;

  if (is_playing || is_paused) {
    ma_sound_get_cursor_in_seconds(&sound, &curr_time);
  }

  return curr_time;
}

float Music::max_time() const {
  float total_time = 0.0f;

  if (is_playing || is_paused) {
    ma_sound_get_length_in_seconds(&sound, &total_time);
  }

  return total_time;
}

void Music::set_position(float seek_point_in_seconds) {
  ma_sound_seek_to_second(&sound, seek_point_in_seconds);
}