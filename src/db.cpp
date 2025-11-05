#include "db.hpp"

#include <sqlite3.h>
#include <taglib/fileref.h>
#include <taglib/tag.h>

#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>

Database::Database() {
  rc = sqlite3_open("music.db", &db);
  sqlite3_exec(db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
  sqlite3_exec(db, "PRAGMA synchronous=NORMAL;", nullptr, nullptr, nullptr);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "Can't open your music files : %s\n", sqlite3_errmsg(db));
    exit(EXIT_FAILURE);
  }

  create_tables();
}

Database::~Database() { sqlite3_close(db); }

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
    sqlite3_free(err_msg);
    exit(EXIT_FAILURE);
  }
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
  }

  sqlite3_finalize(stmt);
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

  } else {
    std::cerr << "Track not found with ID [" << id << "].\n";
    sqlite3_finalize(stmt);
    return 1;
  }

  sqlite3_finalize(stmt);

  return 0;
}

int Database::delete_track(int id) {
  const char *delete_track_sql = "DELETE FROM tracks WHERE id = ?";
  const char *delete_from_playlists_sql =
      "DELETE FROM playlist_tracks WHERE track_id = ?";

  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(db, delete_from_playlists_sql, -1, &stmt, nullptr) ==
      SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
  }

  if (sqlite3_prepare_v2(db, delete_track_sql, -1, &stmt, nullptr) ==
      SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
  } else {
    std::cerr << "Failed to delete track: " << sqlite3_errmsg(db) << std::endl;
    return 1;
  }
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
  if (rc == SQLITE_ROW) {
    state.last_track_id = sqlite3_column_int(stmt, 0);
    state.last_playlist_id = sqlite3_column_int(stmt, 1);
    state.volume = static_cast<float>(sqlite3_column_double(stmt, 2));
    state.if_shuffled = sqlite3_column_int(stmt, 3);
    state.is_repeat = sqlite3_column_int(stmt, 4);
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
}

int Database::add_playlist(const char *playlist_name) {
  const char *sql = "INSERT INTO playlists (name, created_at) VALUES (?,?);";

  sqlite3_stmt *stmt;
  rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db)
              << std::endl;
    return 1;
  }

  sqlite3_bind_text(stmt, 1, playlist_name, -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 2, current_datetime().c_str(), -1, SQLITE_TRANSIENT);

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    std::cerr << "Execution failed: " << sqlite3_errmsg(db) << std::endl;
    sqlite3_finalize(stmt);
    return 1;
  }

  sqlite3_finalize(stmt);
  return 0;
}

int Database::delete_playlist(int id) {
  const char *sql_delete_tracks =
      "DELETE FROM playlist_tracks WHERE playlist_id = ?";
  sqlite3_stmt *stmt;

  rc = sqlite3_prepare_v2(db, sql_delete_tracks, -1, &stmt, nullptr);
  if (rc == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
  } else {
    std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db)
              << std::endl;
    return 1;
  }

  const char *sql_delete_playlist = "DELETE FROM playlists WHERE id = ?";
  rc = sqlite3_prepare_v2(db, sql_delete_playlist, -1, &stmt, nullptr);

  if (rc == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, id);
    rc = sqlite3_step(stmt);
  } else {
    std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db)
              << std::endl;
    return 1;
  }

  if (rc != SQLITE_DONE) {
    std::cerr << "Delete failed: " << sqlite3_errmsg(db) << std::endl;
  }

  sqlite3_finalize(stmt);
  return 0;
}

int Database::add_track_to_playlist(int playlist_id, int track_id,
                                    int position) {
  const char *sql = "INSERT INTO playlist_tracks (playlist_id, track_id, "
                    "position) VALUES (?,?,?);";
  sqlite3_stmt *stmt;

  rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    std::cerr << "Failed to prepare: " << sqlite3_errmsg(db) << std::endl;
    return 1;
  }

  sqlite3_bind_int(stmt, 1, playlist_id);
  sqlite3_bind_int(stmt, 2, track_id);
  sqlite3_bind_int(stmt, 3, position);

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    std::cerr << "Insert failed: " << sqlite3_errmsg(db) << std::endl;
    sqlite3_finalize(stmt);
    return 1;
  }

  sqlite3_finalize(stmt);
  return 0;
}

int Database::remove_track_from_playlist(int playlist_id, int track_id) {
  const char *sql =
      "DELETE FROM playlist_tracks WHERE playlist_id = ? AND track_id =?;";

  sqlite3_stmt *stmt;
  rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

  if (rc != SQLITE_OK) {
    std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db)
              << std::endl;
    return 1;
  }

  sqlite3_bind_int(stmt, 1, playlist_id);
  sqlite3_bind_int(stmt, 2, track_id);

  rc = sqlite3_step(stmt);

  if (rc != SQLITE_DONE) {
    std::cerr << "Delete failed: " << sqlite3_errmsg(db) << std::endl;
  }

  sqlite3_finalize(stmt);
  return 0;
}

int Database::get_next_position(int playlist_id) {
  const char *sql = "SELECT IFNULL(MAX(position), 0) + 1 FROM playlist_tracks "
                    "WHERE playlist_id = ?;";
  sqlite3_stmt *stmt;
  int pos = 1;

  rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db)
              << std::endl;
  }

  sqlite3_bind_int(stmt, 1, playlist_id);
  if (sqlite3_step(stmt) == SQLITE_ROW)
    pos = sqlite3_column_int(stmt, 0);

  sqlite3_finalize(stmt);
  return pos;
}

std::vector<Playlist> Database::get_all_playlist() {
  std::vector<Playlist> playlists;
  const char *sql = "SELECT id, name FROM playlists";

  sqlite3_stmt *stmt;
  rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db)
              << std::endl;
  }

  while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    Playlist pl;
    pl.id = sqlite3_column_int(stmt, 0);
    pl.name = get_text(stmt, 1);
    pl.tracks = get_all_tracks_by_playlist(pl.id);
    playlists.push_back(pl);
  }

  if (rc != SQLITE_DONE) {
    std::cerr << "Select failed: " << sqlite3_errmsg(db) << std::endl;
  }

  sqlite3_finalize(stmt);
  return playlists;
}

std::vector<Track> Database::get_all_tracks_by_playlist(int playlist_id) {
  std::vector<Track> playlist_tracks;
  const char *sql =
      "SELECT t.id, t.file_path, t.title, t.artist, t.duration, t.date_added, "
      "t.last_played, t.play_count FROM playlist_tracks pt JOIN tracks t ON "
      "pt.track_id = t.id WHERE pt.playlist_id = ? ORDER BY pt.position;";

  sqlite3_stmt *stmt;
  rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db)
              << std::endl;
  }

  sqlite3_bind_int(stmt, 1, playlist_id);

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

    playlist_tracks.push_back(t);
  }

  if (rc != SQLITE_DONE) {
    std::cerr << "Select failed: " << sqlite3_errmsg(db) << std::endl;
  }

  sqlite3_finalize(stmt);
  return playlist_tracks;
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