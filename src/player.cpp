#include "../include/player.hpp"
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <stdio.h>
#include <string>
#include <taglib/fileref.h>
#include <thread>
#include <vector>

#include "audio.hpp"
#include "db.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#define IMGUI_IMPL_OPENGL_LOADER_GLAD

int main_window() {
  srand(time(NULL));

  glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
  if (!glfwInit()) {
    fprintf(stderr, "Failed to initialize GLFW\n");
    return -1;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
  GLFWwindow *window = glfwCreateWindow(800, 600, "Music Playr", NULL, NULL);
  if (window == NULL) {
    fprintf(stderr, "Failed to create GLFW window\n");
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    fprintf(stderr, "Failed to initialize GLAD\n");
    return -1;
  }

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;

  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330");

  Database main_database;
  Music main_player;
  AppState state = main_database.load_app_state();
  main_player.set_volume(state.volume);

  std::vector<Track> ALL_TRACKS = main_database.get_all_tracks();
  Track current_song;
  int current_idx = -1;
  bool found = false;

  for (int i = 0; i < ALL_TRACKS.size(); ++i) {
    if (ALL_TRACKS[i].id == state.last_track_id) {
      current_song = ALL_TRACKS[i];
      current_idx = i;
      found = true;
      break;
    }
  }

  if (!found && !ALL_TRACKS.empty()) {
    current_song = ALL_TRACKS.front();
    current_idx = 0;
    found = true;
  }

  auto play_next_track = [&]() {
    if (ALL_TRACKS.empty()) {
      return;
    }

    if (state.is_repeat) {
      main_player.stop();
      main_player.play(current_song.file_path, current_song.id);
    } else if (state.if_shuffled) {
      int new_idx;
      do {
        new_idx = rand() % ALL_TRACKS.size();
      } while (new_idx == current_idx && ALL_TRACKS.size() > 1);

      current_idx = new_idx;
      current_song = ALL_TRACKS[current_idx];
      state.last_track_id = current_song.id;
      main_player.stop();
      main_player.play(current_song.file_path, current_song.id);
    } else {
      current_idx = (current_idx + 1) % ALL_TRACKS.size();
      current_song = ALL_TRACKS[current_idx];
      state.last_track_id = current_song.id;
      main_player.stop();
      main_player.play(current_song.file_path, current_song.id);
    }
  };

  const int target_fps = 28;
  const auto frame_duration = std::chrono::milliseconds(1000 / target_fps);

  while (!glfwWindowShouldClose(window)) {
    auto frame_start = std::chrono::steady_clock::now();

    glfwPollEvents();

    if (main_player.is_finished() && found) {
      play_next_track();
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Player");

    if (found) {
      ImGui::Text("Current song: %s", current_song.title.c_str());
    } else {
      ImGui::Text("No tracks available");
    }

    ImGui::Separator();

    if (ImGui::Button("Play/Pause") && found) {
      main_player.play(current_song.file_path, current_song.id);
    }
    ImGui::SameLine();
    ImGui::Text("State: %s",
                main_player.get_state() == PlaybackState::Playing  ? "Playing"
                : main_player.get_state() == PlaybackState::Paused ? "Paused"
                                                                   : "Stopped");

    ImGui::SameLine();
    if (ImGui::Button("Next")) {
      if (state.is_repeat) {
        state.is_repeat = false;
      }
      play_next_track();
    }

    ImGui::SameLine();
    if (ImGui::Button("Previous")) {
      main_player.stop();
      current_idx = (current_idx - 1 + ALL_TRACKS.size()) % ALL_TRACKS.size();
      current_song = ALL_TRACKS[current_idx];
      state.last_track_id = current_song.id;
      main_player.play(current_song.file_path, current_song.id);
    }

    ImGui::Separator();

    float vol = main_player.get_volume();
    if (ImGui::SliderFloat("Volume", &vol, 0.0f, 1.0f)) {
      main_player.set_volume(vol);
    }
    state.volume = vol;

    static float seek_value = 0.0f;
    static bool is_seeking = false;

    float live = main_player.current_time();
    float total = main_player.max_time();
    if (total <= 0.0f)
      total = 0.0001f;

    if (!is_seeking) {
      seek_value = live;
    }

    ImGui::Separator();

    if (ImGui::SliderFloat("##float", &seek_value, 0.0f, total)) {
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

    ImGui::Separator();

    if (ImGui::Button("Exit")) {
      state.last_track_id = current_song.id;
      state.volume = main_player.get_volume();
      main_database.save_app_state(state);
      exit(EXIT_SUCCESS);
    }

    ImGui::End();

    ImGui::Begin("Queue");

    static char buf[128];
    static ImGuiInputTextFlags flags = ImGuiInputTextFlags_ElideLeft;
    static AudioMetadata _metadata;

    ImGui::InputText("Path to file", buf, IM_ARRAYSIZE(buf), flags);

    ImGui::PushStyleColor(ImGuiCol_Button,
                          (ImVec4)ImColor::HSV(0.7f, 0.6f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                          (ImVec4)ImColor::HSV(0.7f, 0.7f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                          (ImVec4)ImColor::HSV(0.7f, 0.8f, 0.8f));

    std::string buf_path(buf);

    if (ImGui::Button("Add Music")) {
      if (buf_path.empty()) {
        ImGui::OpenPopup("Empty");
      } else if (TagLib::FileRef(buf).isNull()) {
        ImGui::OpenPopup("Wrong");
      } else {
        _metadata = main_database.get_metadata(buf);
        ImGui::OpenPopup("Add");
      }
    }

    ImGui::PopStyleColor(3);

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Add", NULL,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::Text("Confirm Addition?");

      ImGui::Separator();

      ImGui::Text("Path: %s", buf);
      ImGui::Bullet();
      ImGui::Text("Title: %s", _metadata.title.c_str());
      ImGui::Bullet();
      ImGui::Text("Artist: %s", _metadata.artist.c_str());
      ImGui::Bullet();
      ImGui::Text("Length: %ds", _metadata.length_in_seconds);

      ImGui::Separator();

      static bool dont_ask_me_next_time = false;
      ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
      ImGui::Checkbox("Don't ask me next time", &dont_ask_me_next_time);
      ImGui::PopStyleVar();

      if (ImGui::Button("OK", ImVec2(120, 0))) {
        main_database.add_track(buf);
        ImGui::CloseCurrentPopup();
      }
      ImGui::SetItemDefaultFocus();
      ImGui::SameLine();
      if (ImGui::Button("Cancel", ImVec2(120, 0))) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    } else if (ImGui::BeginPopupModal("Empty", NULL,
                                      ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::Text("File Path Empty");
      ImGui::Separator();
      if (ImGui::Button("OK", ImVec2(120, 0))) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::SetItemDefaultFocus();
      ImGui::SameLine();
      if (ImGui::Button("Cancel", ImVec2(120, 0))) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    } else if (ImGui::BeginPopupModal("Wrong", NULL,
                                      ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::TextWrapped("Invalid audio file:");
      ImGui::Bullet();
      ImGui::Text("File may not exist");
      ImGui::Bullet();
      ImGui::Text("Unsupported format");
      ImGui::Bullet();
      ImGui::Text("File may be corrupted");
      ImGui::Separator();
      if (ImGui::Button("OK", ImVec2(120, 0))) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::SetItemDefaultFocus();
      ImGui::SameLine();
      if (ImGui::Button("Cancel", ImVec2(120, 0))) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }

    ImGui::Separator();

    for (const auto &track : ALL_TRACKS) {
      if (ImGui::TreeNode(track.title.c_str())) {
        ImGui::Text("Artist: %s", track.artist.c_str());
        ImGui::Text("Duration: %s", format_time(track.duration).c_str());

        if (ImGui::Button(("Play##" + std::to_string(track.id)).c_str())) {
          current_song = track;
          if ((main_player.get_state() == PlaybackState::Playing) ||
              (main_player.get_state() == PlaybackState::Paused)) {
            main_player.stop();
            main_player.play(current_song.file_path, current_song.id);
          }
        }

        ImGui::TreePop();
        ImGui::Separator();
      }
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

    auto frame_end = std::chrono::steady_clock::now();
    auto elapsed = frame_end - frame_start;

    if (elapsed < frame_duration)
      std::this_thread::sleep_for(frame_duration - elapsed);
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}