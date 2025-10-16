#include "miniaudio/miniaudio.h"
#include <iostream>
#include <ostream>

#define MINIAUDIO_IMPLEMENTATION
#include "audio.hpp"
#include "db.hpp"

void Music::music_menu() {
  while (true) {
    std::vector<Track> all_tracks = _MUSIC_DB.get_all_tracks();
    for (const Track &track : all_tracks) {
      std::cout << "ID:       " << track.id << std::endl;
      std::cout << "Name:     " << track.title << std::endl;
      std::cout << "Artist:   " << track.artist << std::endl;
      std::cout << "Time:     " << track.duration << "s\n";
      std::cout << std::endl;
    }

  choose_song:
    std::cout << "Enter Song ID(0 to quit): ";
    std::cin >> SONG_ID;

    if (SONG_ID == 0) {
      break;
    }

    size_t current_index = -1;
    for (size_t i = 0; i < all_tracks.size(); ++i) {
      if (all_tracks[i].id == SONG_ID) {
        current_index = i;
        break;
      }
    }

    if (current_index == -1) {
      std::cout << "No track found with ID " << SONG_ID << std::endl;
      goto choose_song;
    }

    while (true) {
      Track &current_track = all_tracks[current_index];
      SONG_ID = current_track.id;
      std::cout << "\nPlaying: " << current_track.title << std::endl;

      PlayState user_choice = play_audio(current_track.file_path.c_str());

      if (user_choice == PlayState::NEXT) {
        if (current_index < all_tracks.size() - 1) {
          current_index++;
        } else {
          std::cout << "End of playlist." << std::endl;
          break;
        }
      } else if (user_choice == PlayState::PREV) {
        if (current_index > 0) {
          current_index--;
        } else {
          std::cout << "Already at the first track." << std::endl;
        }
      } else if (user_choice == PlayState::QUIT) {
        break;
      }
    }
  }
}

PlayState Music::play_audio(const char *filepath) {
  ma_sound sound;
  float paused_time = 0.0f;
  ma_result result =
      ma_sound_init_from_file(&engine, filepath, 0, NULL, NULL, &sound);
  if (result != MA_SUCCESS) {
    std::cout << "Failed to load sound: " << filepath << std::endl;
    return PlayState::QUIT;
  }

  ma_sound_seek_to_second(&sound, _MUSIC_DB.last_played_timestamp(SONG_ID));
  ma_sound_start(&sound);
  _MUSIC_DB.increase_play_count(SONG_ID);

  char cmd;
  while (true) {

    if (ma_sound_at_end(&sound)) {
      ma_sound_uninit(&sound);
      return PlayState::NEXT;
    }

    std::cout << "p: pause | r: resume | q: quit | >: next | <: previous\n> ";
    std::cin >> cmd;

    if (cmd == 'p') {
      ma_sound_stop(&sound);
      ma_sound_get_cursor_in_seconds(&sound, &paused_time);
    }

    else if (cmd == 'r')
      ma_sound_start(&sound);
    else if (cmd == '>') {
      ma_sound_uninit(&sound);
      return PlayState::NEXT;
    } else if (cmd == '<') {
      ma_sound_uninit(&sound);
      return PlayState::PREV;
    } else if (cmd == 'q') {
      ma_sound_get_cursor_in_seconds(&sound, &paused_time);
      _MUSIC_DB.add_last_played_timestamp(SONG_ID, paused_time);
      ma_sound_uninit(&sound);
      return PlayState::QUIT;
    }
  }
}