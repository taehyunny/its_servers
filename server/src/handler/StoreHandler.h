#pragma once
#include <memory>
#include "json.hpp"
#include "ClientSession.h"

class StoreHandler {
public: // 상점 목록 요청 처리 함수
    static void handleStoreListRequest(ClientSession* session, const nlohmann::json& requestJson);
};