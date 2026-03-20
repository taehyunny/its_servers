#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <mutex>

class Logger
{
public:
    static void log(const std::string &message)
    {
        static std::mutex logMutex;
        std::lock_guard<std::mutex> lock(logMutex); // 여러 스레드가 동시에 써도 안 꼬이게 보호

        std::ofstream logFile("server_log.txt", std::ios::app); // 실행 폴더에 파일 생성 (이어쓰기 모드)
        if (logFile.is_open())
        {
            logFile << "[" << getCurrentTime() << "] " << message << std::endl;
            logFile.close();
        }
        // 콘솔에도 동시에 찍어주면 보기 편해요!
        std::cout << "[" << getCurrentTime() << "] " << message << std::endl;
    }

private:
    static std::string getCurrentTime()
    {
        time_t now = time(0);
        struct tm tstruct;
        char buf[80];
        tstruct = *localtime(&now);
        strftime(buf, sizeof(buf), "%Y-%m-%d %X", &tstruct);
        return buf;
    }
};