#include "miniaudio/miniaudio.h"
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <ostream>
#include <string>
#include <unistd.h>

#define MINIAUDIO_IMPLEMENTATION
#include "audio.hpp"
#include "logger.hpp"

Music::Music() {
  if (ma_engine_init(NULL, &engine) != MA_SUCCESS) {
    std::cerr << "Failed to init engine\n";
    LOG_ERROR("Failed to initialize miniaudio engine");
  } else {
    LOG_INFO("Miniaudio engine initialized successfully");
  }
  all_track_vector = music_db.get_all_tracks();
};

Music::~Music() {
  ma_sound_uninit(&sound);
  ma_engine_uninit(&engine);
  LOG_INFO("Miniaudio engine cleaned up");
};

void Music::play(const std::string filepath, int track_id) {
  if (state == PlaybackState::Paused) {
    ma_sound_start(&sound);
    state = PlaybackState::Playing;
    LOG_INFO("Resumed playback");
    return;
  }

  if (state == PlaybackState::Playing) {
    pause(track_id);
    return;
  }

  ma_result result =
      ma_sound_init_from_file(&engine, filepath.c_str(), 0, NULL, NULL, &sound);

  if (result != MA_SUCCESS) {
    std::cerr << "Failed to load sound: " << filepath << std::endl;
    LOG_ERROR("Failed to load sound file: " + filepath);
    return;
  }

  ma_sound_seek_to_second(&sound, music_db.last_played_timestamp(track_id));
  ma_sound_start(&sound);
  music_db.increase_play_count(track_id);
  state = PlaybackState::Playing;
  LOG_INFO("Started playing track ID " + std::to_string(track_id) + ": " + filepath);
}

void Music::pause(int track_id) {
  if (ma_sound_is_playing(&sound)) {
    ma_sound_get_cursor_in_seconds(&sound, &paused_time);
    music_db.add_last_played_timestamp(track_id, paused_time);
    ma_sound_stop(&sound);
    state = PlaybackState::Paused;
    LOG_INFO("Paused track ID " + std::to_string(track_id) + " at " + std::to_string(paused_time) + " seconds");
  }
}

void Music::stop() {
  if (state == PlaybackState::Playing || state == PlaybackState::Paused) {
    ma_sound_uninit(&sound);
    state = PlaybackState::Stopped;
    LOG_DEBUG("Stopped playback");
  }
}

bool Music::is_finished() {
  if (state == PlaybackState::Playing && !ma_sound_is_playing(&sound)) {
    state = PlaybackState::Stopped;
    return true;
  }
  return false;
}

void Music::set_volume(float v) {
  if (v < 0.0f)
    v = 0.0f;
  else if (v > 1.0f)
    v = 1.0f;

  volume = v;

  if (state == PlaybackState::Playing || state == PlaybackState::Paused) {
    ma_sound_set_volume(&sound, volume);
  }
  LOG_DEBUG("Volume set to " + std::to_string(volume));
}

float Music::get_volume() const { return volume; }

float Music::current_time() const {
  float curr_time = 0.0f;

  if (state == PlaybackState::Playing || state == PlaybackState::Paused) {
    ma_sound_get_cursor_in_seconds(&sound, &curr_time);
  }

  return curr_time;
}

float Music::max_time() const {
  float total_time = 0.0f;

  if (state == PlaybackState::Playing || state == PlaybackState::Paused) {
    ma_sound_get_length_in_seconds(&sound, &total_time);
  }

  return total_time;
}

void Music::set_position(float seek_point_in_seconds) {
  if (state == PlaybackState::Playing || state == PlaybackState::Paused) {
    ma_sound_seek_to_second(&sound, seek_point_in_seconds);
    LOG_DEBUG("Seeked to position " + std::to_string(seek_point_in_seconds) + " seconds");
  }
}