#include "db.hpp"

#include <sqlite3.h>
#include <taglib/fileref.h>
#include <taglib/tag.h>

#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "logger.hpp"

Database::Database() {
  if (rc != SQLITE_OK) {
    fprintf(stderr, "Can't open your music files : %s\n", sqlite3_errmsg(db));
    LOG_ERROR("Failed to open database: " + std::string(sqlite3_errmsg(db)));
    exit(EXIT_FAILURE);
  }

  LOG_INFO("Database opened successfully: music.db");
  create_tables();
}

Database::~Database() { 
  sqlite3_close(db); 
  LOG_INFO("Database connection closed");
}

void Database::create_tables() {
  const char *sql = "CREATE TABLE IF NOT EXISTS tracks ("
                    "   id INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "   file_path TEXT NOT NULL,"
                    "   title TEXT,"
                    "   artist TEXT,"
                    "   duration REAL,"
                    "   date_added INTEGER,"
                    "   last_played INTEGER,"
                    "   play_count INTEGER DEFAULT 0"
                    ");"

                    "CREATE TABLE IF NOT EXISTS playlists ("
                    "   id INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "   name TEXT NOT NULL,"
                    "   created_at INTEGER"
                    ");"

                    "CREATE TABLE IF NOT EXISTS playlist_tracks("
                    "   playlist_id INTEGER,"
                    "   track_id INTEGER,"
                    "   position INTEGER,"
                    "   FOREIGN KEY(playlist_id) REFERENCES playlists(id),"
                    "   FOREIGN KEY(track_id) REFERENCES tracks(id)"
                    ");"

                    "CREATE TABLE IF NOT EXISTS app_state ("
                    "   id INTEGER PRIMARY KEY CHECK (id = 1),"
                    "   last_track_id INTEGER,"
                    "   last_playlist_id INTEGER,"
                    "   volume REAL DEFAULT 1.0,"
                    "   if_shuffled INTEGER DEFAULT 0,"
                    "   is_repeat INTEGER DEFAULT 0"
                    ");";

  char *err_msg = nullptr;
  int rc = sqlite3_exec(db, sql, nullptr, nullptr, &err_msg);

  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", err_msg);
    LOG_ERROR("Failed to create database tables: " + std::string(err_msg));
    sqlite3_free(err_msg);
    exit(EXIT_FAILURE);
  }
  LOG_INFO("Database tables verified/created successfully");
}

int Database::add_track(const char *_absolute_file_path) {
  music_metadata = get_metadata(_absolute_file_path);
  const char *sql = "INSERT INTO tracks (file_path, title, artist, duration, "
                    "date_added) VALUES (?,?,?,?,?)";

  sqlite3_stmt *stmt;
  rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

  if (rc != SQLITE_OK) {
    std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db)
              << std::endl;
    return 1;
  }

  sqlite3_bind_text(stmt, 1, _absolute_file_path, -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 2, music_metadata.title.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 3, music_metadata.artist.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_int(stmt, 4, music_metadata.length_in_seconds);
  sqlite3_bind_text(stmt, 5, current_datetime().c_str(), -1, SQLITE_TRANSIENT);

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    std::cerr << "Execution failed: " << sqlite3_errmsg(db) << std::endl;
    LOG_ERROR("Failed to insert track: " + std::string(sqlite3_errmsg(db)));
  } else {
    std::cout << "Record inserted successfully!" << std::endl;
    LOG_INFO("Added track to database: " + music_metadata.title + " by " + music_metadata.artist);
  }

  sqlite3_finalize(stmt);
  return 0;
}

std::vector<Track> Database::get_all_tracks() {
  std::vector<Track> tracks;
  const char *sql = "SELECT id, file_path, title, artist, duration, "
                    "date_added, last_played, play_count FROM tracks";

  sqlite3_stmt *stmt;
  rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db)
              << std::endl;
  }

  while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    Track t;
    t.id = sqlite3_column_int(stmt, 0);
    t.file_path = get_text(stmt, 1);
    t.title = get_text(stmt, 2);
    t.artist = get_text(stmt, 3);
    t.duration = sqlite3_column_int(stmt, 4);
    t.date_added = get_text(stmt, 5);
    t.last_played = sqlite3_column_int(stmt, 6);
    t.play_count = sqlite3_column_int(stmt, 7);

    tracks.push_back(t);
  }

  if (rc != SQLITE_DONE) {
    std::cerr << "Select failed: " << sqlite3_errmsg(db) << std::endl;
    LOG_ERROR("Failed to retrieve tracks: " + std::string(sqlite3_errmsg(db)));
  }

  sqlite3_finalize(stmt);
  LOG_DEBUG("Retrieved " + std::to_string(tracks.size()) + " tracks from database");
  return tracks;
}

int Database::get_track_by_id(int id) {
  const char *sql =
      "SELECT file_path, title, artist, play_count FROM tracks WHERE id = ?";

  sqlite3_stmt *stmt;
  rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

  if (rc != SQLITE_OK) {
    std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db)
              << std::endl;
  }

  sqlite3_bind_int(stmt, 1, id);

  rc = sqlite3_step(stmt);
  if (rc == SQLITE_ROW) {
    std::string file_path = get_text(stmt, 0);
    std::string title = get_text(stmt, 1);
    std::string artist = get_text(stmt, 2);
    int play_count = sqlite3_column_int(stmt, 3);

    std::cout << "  Title:  " << title << "\n";
    std::cout << "  Artist: " << artist << "\n";
    std::cout << "  Path:   " << file_path << "\n";
    std::cout << "  Times Played:   " << play_count << "\n";

  } else {
    std::cerr << "Track not found with ID [" << id << "].\n";
    sqlite3_finalize(stmt);
    return 1;
  }

  sqlite3_finalize(stmt);

  return 0;
}

int Database::delete_track(int id) {
  const char *sql = "DELETE FROM tracks WHERE id = ?";

  sqlite3_stmt *stmt;
  rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

  if (rc != SQLITE_OK) {
    std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db)
              << std::endl;
    return 1;
  }

  if (get_track_by_id(id) == 0) {
    sqlite3_bind_int(stmt, 1, id);

    rc = sqlite3_step(stmt);

    if (rc != SQLITE_DONE) {
      std::cerr << "Delete failed: " << sqlite3_errmsg(db) << std::endl;
    } else {
      std::cout << "Track deleted successfully.\n";
      LOG_INFO("Deleted track with ID " + std::to_string(id));
    }

  } else {
    std::cerr << "Failed to Delete: " << sqlite3_errmsg(db) << std::endl;
    sqlite3_finalize(stmt);
    return 1;
  }

  sqlite3_finalize(stmt);
  return 0;
}

int Database::increase_play_count(int id) {
  const char *sql =
      "UPDATE tracks SET play_count = play_count + 1 WHERE id = ?;";

  sqlite3_stmt *stmt;
  rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db)
              << std::endl;
    return 1;
  }

  sqlite3_bind_int(stmt, 1, id);

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    std::cerr << "Update failed: " << sqlite3_errmsg(db) << std::endl;
    LOG_ERROR("Failed to increase play count for track ID " + std::to_string(id));
  } else {
    LOG_DEBUG("Increased play count for track ID " + std::to_string(id));
  }

  sqlite3_finalize(stmt);
  return 0;
}

int Database::add_last_played_timestamp(int id, int time) {
  const char *sql =
      "UPDATE tracks SET last_played = CASE WHEN id = ? THEN ? ELSE 0 END;";

  sqlite3_stmt *stmt;
  rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db)
              << std::endl;
    return 1;
  }

  sqlite3_bind_int(stmt, 1, id);
  sqlite3_bind_int(stmt, 2, time);

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    std::cerr << "Update failed: " << sqlite3_errmsg(db) << std::endl;
    LOG_ERROR("Failed to update last played timestamp for track ID " + std::to_string(id));
  } else {
    LOG_DEBUG("Updated last played timestamp for track ID " + std::to_string(id) + " to " + std::to_string(time) + " seconds");
  }

  sqlite3_finalize(stmt);
  return 0;
}

AudioMetadata Database::get_metadata(const char *file_path) {
  AudioMetadata result;
  TagLib::FileRef file(file_path);

  if (!file.isNull()) {
    result.is_valid = true;
    if (file.tag()) {
      TagLib::Tag *tag = file.tag();
      result.title = tag->title().toCString(true);
      result.artist = tag->artist().toCString(true);
      result.album = tag->album().toCString(true);
      result.year = tag->year();
      result.track = tag->track();
      result.genre = tag->genre().toCString(true);
    }
    if (file.audioProperties()) {
      TagLib::AudioProperties *props = file.audioProperties();
      result.length_in_seconds = props->lengthInSeconds();
      result.bitrate = props->bitrate();
      result.sample_rate = props->sampleRate();
      result.channels = props->channels();
    }
  }

  return result;
}

int Database::last_played_timestamp(int id) {
  const char *sql = "SELECT last_played FROM tracks WHERE id = ?;";

  sqlite3_stmt *stmt;
  rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << '\n';
    return -1;
  }

  sqlite3_bind_int(stmt, 1, id);

  int timestamp = -1;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    timestamp = sqlite3_column_int(stmt, 0);
  } else {
    std::cerr << "Track not found with ID [" << id << "].\n";
  }

  sqlite3_finalize(stmt);
  return timestamp;
}

AppState Database::load_app_state() {
  AppState state{};
  const char *sql = "SELECT last_track_id, last_playlist_id, volume, "
                    "if_shuffled, is_repeat FROM app_state WHERE id = 1;";
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

  if (rc != SQLITE_OK) {
    std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db)
              << std::endl;
    return state;
  }

  rc = sqlite3_step(stmt);
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    state.last_track_id = sqlite3_column_int(stmt, 0);
    state.last_playlist_id = sqlite3_column_int(stmt, 1);
    state.volume = static_cast<float>(sqlite3_column_double(stmt, 2));
    state.if_shuffled = sqlite3_column_int(stmt, 3);
    state.is_repeat = sqlite3_column_int(stmt, 4);
    LOG_DEBUG("Loaded app state from database");
  } else {
    const char *insert_sql =
        "INSERT INTO app_state (id, last_track_id, last_playlist_id, volume, "
        "if_shuffled, is_repeat) "
        "VALUES (1, -1, -1, 0.5, 0, 0);";
    sqlite3_exec(db, insert_sql, nullptr, nullptr, nullptr);

    state.last_track_id = -1;
    state.last_playlist_id = -1;
    state.volume = 1.0f;
    state.if_shuffled = false;
    state.is_repeat = false;
    LOG_INFO("Created default app state in database");
  }

  sqlite3_finalize(stmt);
  return state;
}

void Database::save_app_state(const AppState &s) {
  const char *sql =
      "UPDATE app_state SET last_track_id = ?, last_playlist_id = ?, volume = "
      "?, if_shuffled = ?, is_repeat = ? WHERE id = 1;";

  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

  if (rc != SQLITE_OK) {
    std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db)
              << std::endl;
    return;
  }

  sqlite3_bind_int(stmt, 1, s.last_track_id);
  sqlite3_bind_int(stmt, 2, s.last_playlist_id);
  sqlite3_bind_double(stmt, 3, s.volume);
  sqlite3_bind_int(stmt, 4, s.if_shuffled);
  sqlite3_bind_int(stmt, 5, s.is_repeat);

  sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  // LOG_DEBUG("Saved app state to database");
}

std::string Database::current_datetime() {
  auto now = std::chrono::system_clock::now();
  std::time_t t = std::chrono::system_clock::to_time_t(now);
  std::tm *local = std::localtime(&t);
  std::ostringstream oss;
  oss << std::put_time(local, "%Y-%m-%d %H:%M:%S");
  return oss.str();
}

inline std::string get_text(sqlite3_stmt *stmt, int col) {
  const unsigned char *txt = sqlite3_column_text(stmt, col);
  return txt ? reinterpret_cast<const char *>(txt) : "";
}