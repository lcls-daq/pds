#include "Logger.hh"

using namespace Pds::NsCam;

FormatBackup::FormatBackup(std::ostream& ios) :
  fill_ch_(ios.fill()),
  ios_(ios),
  fmt_(ios.flags())
{}

FormatBackup::~FormatBackup()
{
  restore();
}

void FormatBackup::restore() {
  ios_.fill(fill_ch_);
  ios_.flags(fmt_);
}

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
  out << levelColor(level) << "[" << timestamp() << "] [" << levelStr(level) << "] "
      << message << resetColor() << "\n";
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

const char* Logger::levelStr(Level level)
{
  switch (level) {
    case Level::DEBUG: return "DEBUG";
    case Level::INFO:  return "INFO ";
    case Level::WARN:  return "WARN ";
    case Level::ERROR: return "ERROR";
    case Level::OFF:   return "OFF  ";
    default:           return "?????";
  }
}

const char* Logger::levelColor(Level level)
{
  switch (level) {
    case Level::DEBUG: return "\033[34m";
    case Level::INFO:  return "\033[32m";
    case Level::WARN:  return "\033[33m";
    case Level::ERROR: return "\033[31m";
    default:           return resetColor();
  }
}

const char* Logger::resetColor()
{
  return "\033[0m";
}

LogStream::LogStream(Logger::Level level) :
  level_(level)
{}

LogStream::~LogStream()
{
  Logger::instance().log(level_, stream_.str());
}

std::string StreamHelper::str() const
{
  return stream_.str();
}
