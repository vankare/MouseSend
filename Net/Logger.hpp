#pragma once


#include <ctime>
#include <sys/time.h>
#include <unistd.h>

#include <mutex>
#include <string>
#include <memory>
#include <sstream>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <unordered_map>
namespace NS_LOGGER_MUDULE
{
    enum class LogLevel
    {
        INFO,
        WARNING,
        ERRNO,
        FATAL,
        DEBUG
    };

    class LogStrategy
    {
    public:
        LogStrategy(){  } 
        virtual void SyncLog(const std::string message) = 0;
    protected:
        std::mutex mutex;
    };

    class ConsoleStrategy : public LogStrategy
    {
    public:
        ConsoleStrategy() { }
        void SyncLog(const std::string message)
        {
            std::unique_lock<std::mutex> lock(mutex);
            std::cerr << message << '\n';
        }
    };

    class FileStrategy : public LogStrategy
    {
    public:
        FileStrategy(std::string path = "./log/", std::string filename = "log.txt")
            :_path(path)
            ,_filename(filename)
        {
            if(!_path.empty() && _path.back() != '/')
            {
                _path += '/';
            }
            try
            {
                std::filesystem::create_directories(_path);
            }
            catch(const std::filesystem::filesystem_error& e)
            {
                std::cerr << e.what() << '\n';
            }
        }
        void SyncLog(const std::string message) override
        {
            std::unique_lock<std::mutex> lock(mutex);
            std::ofstream out(_path + _filename, std::ios::app);
            if(!out.is_open())
            {
                std::cerr << "ofstream " << _path << _filename << '\n';
            }
            out << message << '\n';
            out.close();
        }
    private:
        std::string _path;
        std::string _filename;
    };

    const std::unordered_map<LogLevel, std::string> map = {
        {LogLevel::INFO, "INFO"},
        {LogLevel::WARNING, "WARNING"},
        {LogLevel::ERRNO, "ERRNO"},
        {LogLevel::FATAL, "FATAL"},
        {LogLevel::DEBUG, "DEBUG"}
    };

    std::string GetCurTime()
    {
        time_t t_t = time(0);
        tm t_m;
        localtime_r(&t_t, &t_m);

        char retstr[128];
        snprintf(retstr, sizeof(retstr), "%04d-%02d-%02d:%02d:%02d:%02d",
                t_m.tm_year + 1900,
                t_m.tm_mon,
                t_m.tm_mday,
                t_m.tm_hour,
                t_m.tm_min,
                t_m.tm_sec
                );
        return std::string(retstr);
    }

    class Logger
    {
    private:
        class LogMessage
        {
        public:
            LogMessage(LogLevel level, std::string& file, int line, Logger& logger)
                :_curtime(GetCurTime())
                ,_level(level)
                ,_pid(getpid())
                ,_file(file)
                ,_line(line)
                ,_logger(logger)
            {
                std::stringstream ss;
                ss << "[" << _curtime << "] " <<
                    "[" << map.at(_level) << "] " <<
                    "[" << _pid << "] " <<
                    "[" << file << "] " <<
                    "[" << line << "] ";

                _loginfo = ss.str();
            }
            template<class T>
            LogMessage& operator<<(const T& info)
            {
                std::stringstream ss; 
                ss << info;
                _loginfo += ss.str();
                return *this;
            }
            ~LogMessage()
            {
                _logger._strategy->SyncLog(_loginfo);
            }
        private:
            std::string _curtime;
            LogLevel _level;
            pid_t _pid;
            std::string _file;
            int _line;
            Logger& _logger;
            std::string _loginfo;
        };
    public:
        Logger()
        {
            UseConsoleStrategy();
        }
        void UseConsoleStrategy()
        {
            _strategy = std::make_unique<ConsoleStrategy>();
        }
        void UseFileStrategy()
        {
            _strategy = std::make_unique<FileStrategy>();
        }

        LogMessage operator()(LogLevel level, std::string file, int line)
        {
            return LogMessage(level, file, line, *this);
        }
    private:
        std::unique_ptr<LogStrategy> _strategy;
    };
}

inline NS_LOGGER_MUDULE::Logger logger;

#define ENABLE_CONSOLE_STRATEGY() logger.UseConsoleStrategy()
#define ENABLE_FILE_STRATEGY() logger.UseFileStrategy()

#define LOG(level) logger(level, __FILE__, __LINE__)

