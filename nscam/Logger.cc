#include "Logger.hh"

using namespace Pds::NsCam;

Logger& Logger::instance()
{
  static Logger inst;
  return inst;
}

void Logger::setLevel(Level level)
{
  std::lock_guard<std::mutex> lock(mutex_);
  level_ = level;
}

Logger::Level Logger::getLevel() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return level_;
}

void Logger::log(Level level, const std::string& message) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (level < level_) return;

  // DEBUG/INFO -> stdout, WARN/ERROR -> stderr
  std::ostream& out = (level >= Level::WARN) ? std::cerr : std::cout;
  out << "[" << timestamp() << "] [" << levelStr(level) << "] "
      << message << "\n";
  // Flush stderr immediately so errors are never lost
  if (level >= Level::WARN) out.flush();
}

void Logger::log(Level level, const std::string& prefix, const std::string& message) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (level < level_) return;

  // DEBUG/INFO -> stdout, WARN/ERROR -> stderr
  std::ostream& out = (level >= Level::WARN) ? std::cerr : std::cout;
  out << "[" << timestamp() << "] [" << levelStr(level) << "] "
      << prefix << ": " << message << "\n";
  // Flush stderr immediately so errors are never lost
  if (level >= Level::WARN) out.flush();
}

void Logger::debug(const std::string& msg)
{
  log(Level::DEBUG, msg);
}

void Logger::info(const std::string& msg)
{
  log(Level::INFO,  msg);
}

void Logger::warn(const std::string& msg)
{
  log(Level::WARN,  msg);
}

void Logger::error(const std::string& msg)
{
  log(Level::ERROR, msg);
}

Logger::Logger() :
  level_(Level::INFO)
{}

std::string Logger::timestamp() {
  std::time_t t = std::time(nullptr);
  std::tm tm_buf{};
#ifdef _WIN32
  localtime_s(&tm_buf, &t);
#else
  localtime_r(&t, &tm_buf);
#endif
  char buf[20];
  std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_buf);
  return buf;
}

const char* Logger::levelStr(Level level) {
  switch (level) {
    case Level::DEBUG: return "DEBUG";
    case Level::INFO:  return "INFO ";
    case Level::WARN:  return "WARN ";
    case Level::ERROR: return "ERROR";
    default:           return "?????";
  }
};
