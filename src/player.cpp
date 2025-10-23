#include "../include/player.hpp"
#include <cstdio>
#include <cstdlib>
#include <stdio.h>
#include <string>
#include <vector>

#include "audio.hpp"
#include "db.hpp"
#include "logger.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#define IMGUI_IMPL_OPENGL_LOADER_GLAD

int main_window() {
  LOG_INFO("Initializing GLFW");
  glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
  if (!glfwInit()) {
    fprintf(stderr, "Failed to initialize GLFW\n");
    LOG_ERROR("Failed to initialize GLFW");
    return -1;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
  LOG_INFO("Creating GLFW window (800x600)");
  GLFWwindow *window = glfwCreateWindow(800, 600, "Music Playr", NULL, NULL);
  if (window == NULL) {
    fprintf(stderr, "Failed to create GLFW window\n");
    LOG_ERROR("Failed to create GLFW window");
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);
  LOG_INFO("Initializing GLAD");
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    fprintf(stderr, "Failed to initialize GLAD\n");
    LOG_ERROR("Failed to initialize GLAD");
    return -1;
  }

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;

  ImGui::StyleColorsDark();

  LOG_INFO("Initializing ImGui with GLFW and OpenGL3 backends");
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330");

  LOG_INFO("Initializing Database");
  Database main_database;
  Music main_player;
  AppState state = main_database.load_app_state();
  LOG_INFO("Loaded app state - Last Track ID: " + std::to_string(state.last_track_id) + 
           ", Volume: " + std::to_string(state.volume) + 
           ", Shuffled: " + std::to_string(state.if_shuffled) + 
           ", Repeat: " + std::to_string(state.is_repeat));
  main_player.set_volume(state.volume);

  std::vector<Track> ALL_TRACKS = main_database.get_all_tracks();
  LOG_INFO("Loaded " + std::to_string(ALL_TRACKS.size()) + " tracks from database");
  Track current_song;
  int current_idx = -1;
  bool found = false;

  for (int i = 0; i < ALL_TRACKS.size(); ++i) {
    if (ALL_TRACKS[i].id == state.last_track_id) {
      current_song = ALL_TRACKS[i];
      current_idx = i;
      found = true;
      LOG_INFO("Restored last played track: " + current_song.title + " by " + current_song.artist);
      break;
    }
  }

  if (!found && !ALL_TRACKS.empty()) {
    current_song = ALL_TRACKS.front();
    found = true;
    LOG_INFO("No previous track found, using first track: " + current_song.title);
  }

  LOG_INFO("Starting main event loop");
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Player");

    if (main_player.is_finished()) {
      if (state.is_repeat) {
        LOG_DEBUG("Track finished - Repeating: " + current_song.title);
        main_player.play(current_song.file_path, current_song.id);
      } else if (state.if_shuffled) {
        current_idx = rand() % ALL_TRACKS.size();
        current_song = ALL_TRACKS[current_idx];
        state.last_track_id = current_song.id;
        main_database.save_app_state(state);
        LOG_INFO("Track finished - Playing shuffled track: " + current_song.title);
        main_player.play(current_song.file_path, current_song.id);
      } else {
        current_idx = (current_idx + 1) % ALL_TRACKS.size();
        current_song = ALL_TRACKS[current_idx];
        state.last_track_id = current_song.id;
        main_database.save_app_state(state);
        LOG_INFO("Track finished - Playing next track: " + current_song.title);
        main_player.play(current_song.file_path, current_song.id);
      }
    }

    if (found) {
      ImGui::Text("Current song: %s", current_song.title.c_str());
    } else {
      ImGui::Text("No tracks available");
    }

    if (ImGui::Button("Play/Pause") && found) {
    play_from_selectable:
      main_player.play(current_song.file_path, current_song.id);
    }
    ImGui::SameLine();
    ImGui::Text("State: %s",
                main_player.get_state() == PlaybackState::Playing  ? "Playing"
                : main_player.get_state() == PlaybackState::Paused ? "Paused"
                                                                   : "Stopped");

    ImGui::SameLine();
    if (ImGui::Button("Next")) {
      main_player.stop();
      current_idx = (current_idx + 1) % ALL_TRACKS.size();
      current_song = ALL_TRACKS[current_idx];
      state.last_track_id = current_song.id;
      main_database.save_app_state(state);
      LOG_INFO("User pressed Next - Playing: " + current_song.title);
      main_player.play(current_song.file_path, current_song.id);
    }
    ImGui::SameLine();
    if (ImGui::Button("Previous")) {
      main_player.stop();
      current_idx = (current_idx - 1 + ALL_TRACKS.size()) % ALL_TRACKS.size();
      current_song = ALL_TRACKS[current_idx];
      state.last_track_id = current_song.id;
      main_database.save_app_state(state);
      LOG_INFO("User pressed Previous - Playing: " + current_song.title);
      main_player.play(current_song.file_path, current_song.id);
    }

    float vol = main_player.get_volume();
    if (ImGui::SliderFloat("Volume", &vol, 0.0f, 1.0f)) {
      main_player.set_volume(vol);
    }
    state.volume = vol;
    main_database.save_app_state(state);

    static float seek_value = 0.0f;
    static bool is_seeking = false;

    float live = main_player.current_time();
    float total = main_player.max_time();
    if (total <= 0.0f)
      total = 0.0001f;

    if (!is_seeking) {
      seek_value = live;
    }

    if (ImGui::SliderFloat("Seek", &seek_value, 0.0f, total)) {
      is_seeking = true;
    }

    if (ImGui::IsItemDeactivatedAfterEdit()) {
      main_player.set_position(seek_value);
      is_seeking = false;
    } else if (!ImGui::IsItemActive()) {
      is_seeking = false;
    }

    auto format_time = [](float seconds) {
      int mins = static_cast<int>(seconds) / 60;
      int secs = static_cast<int>(seconds) % 60;
      char buf[16];
      snprintf(buf, sizeof(buf), "%02d:%02d", mins, secs);
      return std::string(buf);
    };
    ImGui::SameLine();
    ImGui::Text("%s / %s", format_time(live).c_str(),
                format_time(total).c_str());

    ImGui::Checkbox("Shuffle", &state.if_shuffled);
    ImGui::Checkbox("Repeat", &state.is_repeat);
    if (ImGui::IsItemEdited()) {
      main_database.save_app_state(state);
    }

    if (ImGui::Button("Exit")) {
      state.last_track_id = current_song.id;
      state.volume = main_player.get_volume();
      main_database.save_app_state(state);
      LOG_INFO("User pressed Exit - Saving state and closing application");
      exit(EXIT_SUCCESS);
    }

    ImGui::End();

    ImGui::Begin("Queue");

    for (const auto &track : ALL_TRACKS) {
      if (ImGui::Selectable(track.title.c_str())) {
        current_song = track;
        LOG_INFO("User selected track from queue: " + track.title + " by " + track.artist);
        goto play_from_selectable;
      }
      ImGui::Text("%s", track.artist.c_str());
      ImGui::Text("%s", format_time(track.duration).c_str());
    }

    ImGui::End();

    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);
  glfwTerminate();
  LOG_INFO("GLFW and ImGui cleaned up successfully");
  return 0;
}