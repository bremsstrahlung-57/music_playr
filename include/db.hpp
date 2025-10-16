#pragma once
#include <sqlite3.h>

#include <string>
#include <vector>

struct AudioMetadata {
  std::string title;
  std::string artist;
  std::string album;
  std::string genre;
  unsigned int year = 0;
  unsigned int track = 0;
  int length_in_seconds = 0;
  int bitrate = 0;
  int sample_rate = 0;
  int channels = 0;
  bool is_valid = false;
};

struct Track {
  int id;
  std::string file_path;
  std::string title;
  std::string artist;
  int duration;
  std::string date_added;
  int last_played;
  int play_count;
};

class Database {
private:
  sqlite3 *db;
  AudioMetadata music_metadata;
  const char *music_database = "music.db";
  int rc = sqlite3_open(music_database, &db);
  void create_tables();

public:
  Database();
  ~Database();

  std::string current_datetime();
  AudioMetadata get_metadata(const char *file_path);

  int add_track(const char *__absolute_file_path);
  std::vector<Track> get_all_tracks();
  int get_track_by_id(int id);
  int delete_track(int id);
  int increase_play_count(int id);
};

inline std::string get_text(sqlite3_stmt *stmt, int col);