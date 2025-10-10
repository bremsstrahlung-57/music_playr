#include <iostream>

#include "audio.hpp"
#include "db.hpp"
#include "player.hpp"

int main() {
    Database MusicDB;
    music start;
    // MusicDB.add_track("/home/sagar/Projects/music_playr/assets/take_me_away.mp3");
    std::vector<Track> t = MusicDB.get_all_tracks();
    for (const auto& track : t) {
        std::cout << "ID: " << track.id << std::endl;
        std::cout << "File Path: " << track.file_path << std::endl;
        std::cout << "Title: " << track.title << std::endl;
        std::cout << "Artist: " << track.artist << std::endl;
        std::cout << "Duration: " << track.duration << std::endl;
        std::cout << "Date Added: " << track.date_added << std::endl;
        std::cout << "Last Played: " << track.last_played << std::endl;
        std::cout << "Play Count: " << track.play_count << std::endl;
    }
    // start.music_list();
    // start.play_audio(false);
    // newWindow();
    return 0;
}
