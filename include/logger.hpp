#pragma once

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>

enum class LogLevel { INFO, WARNING, ERROR, DEBUG };

class Logger {
private:
  std::ofstream log_file;
  std::mutex log_mutex;
  std::string log_file_path;
  bool enabled;

  std::string get_timestamp() const;
  std::string level_to_string(LogLevel level) const;

public:
  Logger(const std::string &filename = "music_playr.log");
  ~Logger();

  void log(LogLevel level, const std::string &message);
  void info(const std::string &message);
  void warning(const std::string &message);
  void error(const std::string &message);
  void debug(const std::string &message);

  void enable();
  void disable();
  bool is_enabled() const;

  // Static instance for global access
  static Logger &get_instance();
};

// Convenience macros for logging
#define LOG_INFO(msg) Logger::get_instance().info(msg)
#define LOG_WARNING(msg) Logger::get_instance().warning(msg)
#define LOG_ERROR(msg) Logger::get_instance().error(msg)
#define LOG_DEBUG(msg) Logger::get_instance().debug(msg)
