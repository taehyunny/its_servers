#include "TimeUtil.h"
#include <chrono>
#include <iomanip>
#include <sstream>

// 🚀 1. 현재 날짜와 시간 (YYYY-MM-DD HH:MM:SS) - 멤버십 기록용
std::string TimeUtil::getCurrentDateTime()
{
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

// 🚀 2. 한 달 뒤 만료일 계산
std::string TimeUtil::getExpiryDate()
{
    auto now = std::chrono::system_clock::now();
    auto expiry = now + std::chrono::hours(24 * 30);
    auto in_time_t = std::chrono::system_clock::to_time_t(expiry);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

// 🚀 3. 현재 날짜만 (YYYY-MM-DD)
std::string TimeUtil::getCurrentDate()
{
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d");
    return ss.str();
}