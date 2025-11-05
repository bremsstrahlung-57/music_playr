#pragma once

#include "audio.hpp"
#include "db.hpp"
#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include <string>

int main_window();
void render_track_list(Database &main_database, Music &main_player,
                       std::vector<Track> &ALL_TRACKS,
                       std::vector<Playlist> &ALL_PLAYLISTS,
                       Track &current_song);
void render_playlist(Database &main_database, Music &main_player,
                     std::vector<Playlist> &ALL_PLAYLISTS,
                     std::vector<Track> &ALL_TRACKS, Track &current_song);
void render_playlist_track_list(Database &main_database, Music &main_player,
                                std::vector<Track> &ALL_TRACKS,
                                std::vector<Playlist> &ALL_PLAYLISTS,
                                Track &current_song,
                                Playlist &current_playlist);
std::string format_time(float seconds);