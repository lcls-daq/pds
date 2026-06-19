#ifndef Pds_NsCam_Logger_hh
#define Pds_NsCam_Logger_hh

#include <ctime>
#include <iostream>
#include <mutex>
#include <string>

namespace Pds {
  namespace NsCam {
    class Logger {
    public:
      enum class Level { DEBUG = 0, INFO = 1, WARN = 2, ERROR = 3 };

      static Logger& instance();

      void setLevel(Level level);

      Level getLevel() const;

      void log(Level level, const std::string& message);
      void log(Level level, const std::string& prefix, const std::string& message);

      // Convenience wrappers
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

      // Non-copyable
      Logger(const Logger&)            = delete;
      Logger& operator=(const Logger&) = delete;

    private:
      Logger();

      mutable std::mutex mutex_;
      Level              level_;

      static std::string timestamp();

      static const char* levelStr(Level level);
    };
  }
}

// helper macros
#ifndef _NOLOGGERMACROS
#define LOG_DEBUG(msg) Pds::NsCam::Logger::instance().debug(msg)
#define LOG_INFO(msg)  Pds::NsCam::Logger::instance().info(msg)
#define LOG_WARN(msg)  Pds::NsCam::Logger::instance().warn(msg)
#define LOG_ERROR(msg) Pds::NsCam::Logger::instance().error(msg)
#define LOG_EXCEPTION(exp) Pds::NsCam::Logger::instance().exception(exp)
#endif

#endif
