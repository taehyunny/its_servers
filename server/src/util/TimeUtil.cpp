#include "TimeUtil.h"
#include <chrono>
#include <iomanip>
#include <sstream>

std::string TimeUtil::getCurrentDate()
{
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    // 태현님이 원하는 형식대로 ("%Y-%m-%d" 등)
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d");
    return ss.str();
}