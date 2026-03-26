#pragma once
#include <string>
#include <vector>
#include "json.hpp"

// 🏍️ 4010: REQ_DELIVERY_COMPLETE (라이더 -> 서버)
struct ReqDeliveryCompleteDTO
{
    std::string orderId;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ReqDeliveryCompleteDTO, orderId)
};

// 🏍️ 4011: RES_DELIVERY_COMPLETE (서버 -> 라이더)
struct ResDeliveryCompleteDTO
{
    int status; // 200: 성공, 500: 에러
    std::string message;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ResDeliveryCompleteDTO, status, message)
};