#pragma once

#include <fstream>
#include <utility>
#include <ios>
#include <string>
#include <sys/stat.h>
#include <ctime>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <chrono>
#include <atomic>
#include <vector>


namespace NS_UTIL_MODULE
{
    const std::string temp = "./temp/";

    class TimeUtil
    {
    public:
        static std::string GetDateTime()
        {
            std::time_t now = std::time(nullptr);
            std::tm tm_buf{};
            localtime_r(&now, &tm_buf);

            char buf[64];
            std::snprintf(buf, sizeof(buf),
                    "%04d-%02d-%02d %02d:%02d:%02d",
                    tm_buf.tm_year + 1900,
                    tm_buf.tm_mon + 1,
                    tm_buf.tm_mday,
                    tm_buf.tm_hour,
                    tm_buf.tm_min,
                    tm_buf.tm_sec
                    );
            return std::string(buf);
        }
        static std::string GetTimeMs()
        {
            struct timeval tv;
            gettimeofday(&tv, nullptr);
            time_t sec = tv.tv_sec;
            suseconds_t sus = tv.tv_usec;
            long long ms = sec * 1000 + sus / 1000;
            return std::to_string(ms);
        }

        template<class DurationType>
        static struct timeval ToTimeval(DurationType d)
        {
            auto sec = std::chrono::duration_cast<std::chrono::seconds>(d);
            auto usec = std::chrono::duration_cast<std::chrono::microseconds>(d - sec);

            struct timeval tv;
            tv.tv_sec = static_cast<time_t>(sec.count());
            tv.tv_usec = static_cast<suseconds_t>(usec.count());

            return tv;
        }
    };

    class FileUtil
    {
    public:
        static bool IsExists(const std::string& file)
        {
            struct stat st;
            int n = stat(file.c_str(), &st);
            if(n == -1)
                return false;
            else
                return true;
        }
        static std::string UniqueFileName()
        {
            static std::atomic_int id(0);
            id++;
            std::string time = TimeUtil::GetTimeMs();
            std::string uniuq_id = std::to_string(id);
            return time + "_" + uniuq_id;
        }

        static bool WriteFile(const std::string& filename, const std::vector<char>& content)
        {
            std::ofstream out(filename, std::ios::out);
            if(!out.is_open())
            {
                return false;
            }
            out.write(content.data(), content.size());

            return true;
        }
        static std::vector<char> ReadFile(const std::string& filename)
        {
            std::ifstream in(filename, std::ios::in);
            if(!in.is_open())
            {
                return std::vector<char>();
            }

            std::vector<char> content;

            in.seekg(0, std::ios::end);
            size_t file_size = in.tellg();
            in.seekg(0, std::ios::beg);

            content.resize(file_size);
            in.read(content.data(), file_size);

            return content;
        }
    };
    
    class StringUtil
    {
    public:
        static void Split(const std::string& str, std::vector<std::string>* vec, const std::string& sep)
        {
            size_t pos = str.find(sep);
            size_t prev = 0;
            while(pos != std::string::npos)
            {
                vec->push_back(str.substr(prev, pos - prev));
                prev = pos + sep.size();
                pos = str.find(sep, prev);
            }
            vec->push_back(str.substr(prev));
        }
    };
}
