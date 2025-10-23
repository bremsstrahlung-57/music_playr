#include "logger.hpp"
#include <iostream>

Logger::Logger(const std::string &filename)
    : log_file_path(filename), enabled(true) {
  log_file.open(log_file_path, std::ios::app);
  if (!log_file.is_open()) {
    std::cerr << "Failed to open log file: " << log_file_path << std::endl;
    enabled = false;
  } else {
    log(LogLevel::INFO, "=== Music Player Started ===");
  }
}

Logger::~Logger() {
  if (log_file.is_open()) {
    log(LogLevel::INFO, "=== Music Player Stopped ===");
    log_file.close();
  }
}

std::string Logger::get_timestamp() const {
  auto now = std::chrono::system_clock::now();
  std::time_t t = std::chrono::system_clock::to_time_t(now);
  std::tm *local = std::localtime(&t);

  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) %
            1000;

  std::ostringstream oss;
  oss << std::put_time(local, "%Y-%m-%d %H:%M:%S");
  oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
  return oss.str();
}

std::string Logger::level_to_string(LogLevel level) const {
  switch (level) {
  case LogLevel::INFO:
    return "INFO";
  case LogLevel::WARNING:
    return "WARNING";
  case LogLevel::ERROR:
    return "ERROR";
  case LogLevel::DEBUG:
    return "DEBUG";
  default:
    return "UNKNOWN";
  }
}

void Logger::log(LogLevel level, const std::string &message) {
  if (!enabled || !log_file.is_open()) {
    return;
  }

  std::lock_guard<std::mutex> lock(log_mutex);
  log_file << "[" << get_timestamp() << "] [" << level_to_string(level)
           << "] " << message << std::endl;
  log_file.flush();
}

void Logger::info(const std::string &message) { log(LogLevel::INFO, message); }

void Logger::warning(const std::string &message) {
  log(LogLevel::WARNING, message);
}

void Logger::error(const std::string &message) {
  log(LogLevel::ERROR, message);
}

void Logger::debug(const std::string &message) {
  log(LogLevel::DEBUG, message);
}

void Logger::enable() { enabled = true; }

void Logger::disable() { enabled = false; }

bool Logger::is_enabled() const { return enabled; }

Logger &Logger::get_instance() {
  static Logger instance;
  return instance;
}
