#ifndef Pds_NsCam_Logger_hh
#define Pds_NsCam_Logger_hh

#include <ctime>
#include <iostream>
#include <iomanip>
#include <mutex>
#include <string>
#include <sstream>

namespace Pds {
  namespace NsCam {
    class FormatBackup {
    public:
      FormatBackup(std::ostream& ios);
      ~FormatBackup();

      void restore();

      FormatBackup(const FormatBackup &rhs) = delete;
      FormatBackup& operator= (const FormatBackup& rhs) = delete;

      static std::ostream& initFormat(std::ostream& ios);
      static std::ostream& hexOn(std::ostream& ios);
      static std::ostream& hexOff(std::ostream& ios);
      static std::ostream& pad(std::ostream& ios);
      static std::string section(const std::string& title);

      static constexpr int InfoPrintWidth   = 25;
      static constexpr int InfoPrintLeftPad = 1;
      static const std::string InfoLeftPad;
      static const std::string InfoElemSeparator;

    private:
      char fill_ch_;
      std::ostream& ios_;
      std::ios::fmtflags fmt_;
    };

    class Logger {
    public:
      enum class Level : unsigned { DEBUG = 0, INFO = 1, WARN = 2, ERROR = 3, OFF = 4 };

      static Logger& instance();

      void setLevel(Level level);

      Level getLevel() const;

      void log(Level level, const std::string& message);

      // Convenience wrappers for logging a throwing exception
      void debug(const std::string& msg);
      void info (const std::string& msg);
      void warn (const std::string& msg);
      void error(const std::string& msg);
      template <typename E> 
      void exception(const E& exp) {
        static_assert(std::is_base_of<std::exception, E>::value, "E must be a derived class of std::exception.");
        log(Level::ERROR, exp.what());
        throw exp;
      }
      static std::string toString(const std::ostringstream& oss);

      // Non-copyable
      Logger(const Logger&)            = delete;
      Logger& operator=(const Logger&) = delete;

    private:
      Logger();

      mutable std::mutex mutex_;
      Level              level_;

      static std::string timestamp();

      static const char* levelStr(Level level);
      static const char* levelColor(Level level);
      static const char* resetColor();
    };

    class LogStream {
    public:
      LogStream(Logger::Level level);
      ~LogStream();

      template <typename T>
      LogStream& operator<<(const T& value) {
        stream_ << value;
        return *this;
      }

    private:
      std::ostringstream stream_;
      Logger::Level level_;
    };

    class StreamHelper {
    public:
      template <typename T>
      StreamHelper& operator<<(const T& value) {
        stream_ << value;
        return *this;
      }

      std::string str() const;

    private:
      std::ostringstream stream_;
    };
  }
}

// helper macros
#ifndef _NOLOGGERMACROS
#define LOG_DEBUG LogStream(Logger::Level::DEBUG)
#define LOG_INFO  LogStream(Logger::Level::INFO)
#define LOG_WARN  LogStream(Logger::Level::WARN)
#define LOG_ERROR LogStream(Logger::Level::ERROR)
#define LOG_EXCEPTION(exp, ...) Pds::NsCam::Logger::instance().exception(exp(__VA_ARGS__))
#define LOG_STR(stream) (StreamHelper() << stream).str()
#define INFO_FLT_PREC(prec) std::fixed << std::setprecision(prec)
#define INFO_HEADER(msg, prec) FormatBackup::initFormat << msg + FormatBackup::InfoElemSeparator << "\n" << FormatBackup::section(msg) << INFO_FLT_PREC(prec)
#define INFO_PAD(msg) FormatBackup::pad << msg + FormatBackup::InfoElemSeparator
#define INFO_HEX(value) FormatBackup::hexOn << value << FormatBackup::hexOff
#define INFO_INDENT FormatBackup::InfoLeftPad << FormatBackup::InfoLeftPad
#endif

#endif
