#pragma once
#include "json.hpp"
#include "ClientSession.h"

class MenuHandler {
public:
    // 앞서 경험한 대로 스마트 포인터 대신 일반 포인터(*)를 사용합니다.
    static void handleMenuListRequest(ClientSession* session, const nlohmann::json& requestJson);
};