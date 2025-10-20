#include "../include/player.hpp"
#include <cstdio>
#include <cstdlib>
#include <stdio.h>
#include <string>
#include <vector>

#include "audio.hpp"
#include "db.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#define IMGUI_IMPL_OPENGL_LOADER_GLAD

int main_window() {
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

  std::vector<Track> ALL_TRACKS = main_database.get_all_tracks();
  Track current_song;
  int current_idx = -1;
  bool found = false;

  for (const auto &track : ALL_TRACKS) {
    if (track.last_played != 0) {
      current_song = track;
      found = true;
      break;
    }
    current_idx++;
  }

  if (!found && !ALL_TRACKS.empty()) {
    current_song = ALL_TRACKS.front();
    found = true;
  }

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Player");

    if (found) {
      ImGui::Text("Current song: %d : %s", current_song.id,
                  current_song.title.c_str());
    } else {
      ImGui::Text("No tracks available");
    }

    if (ImGui::Button("Play/Pause") && found) {
    play_from_selectable:
      main_player.play(current_song.file_path, current_song.id);
    }
    ImGui::SameLine();
    if (ImGui::Button("Next")) {
      main_player.stop();
      current_idx = (current_idx + 1) % ALL_TRACKS.size();
      current_song = ALL_TRACKS[current_idx];
      main_player.play(current_song.file_path, current_song.id);
    }
    ImGui::SameLine();
    if (ImGui::Button("Previous")) {
      main_player.stop();
      current_idx = (current_idx - 1 + ALL_TRACKS.size()) % ALL_TRACKS.size();
      current_song = ALL_TRACKS[current_idx];
      main_player.play(current_song.file_path, current_song.id);
    }

    float vol = main_player.get_volume();
    if (ImGui::SliderFloat("Volume", &vol, 0.0f, 1.0f)) {
      main_player.set_volume(vol);
    }

    if (ImGui::Button("Exit")) {
      exit(EXIT_SUCCESS);
    }

    for (const auto &track : ALL_TRACKS) {
      if (ImGui::Selectable(track.title.c_str())) {
        current_song = track;
        goto play_from_selectable;
      }
      ImGui::Text("%s", track.artist.c_str());
      ImGui::Text("%d", track.duration);
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
  return 0;
}