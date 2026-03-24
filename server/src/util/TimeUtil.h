#pragma once
#include <string>

class TimeUtil
{
public:
    static std::string getCurrentDateTime(); // "YYYY-MM-DD HH:MM:SS" 반환
    static std::string getExpiryDate();      // 현재 시간으로부터 30일 후의 날짜를 "YYYY-MM-DD HH:MM:SS" 형식으로 반환
    static std::string getCurrentDate();     // "YYYY-MM-DD" 반환
};
