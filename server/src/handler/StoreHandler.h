#pragma once
#include <memory>
#include <string>
#include "json.hpp"
#include "ClientSession.h"

class StoreHandler
{
public:
    // 🚀 스마트 포인터와 string으로 통일! (Dispatcher와 완벽 호환)
    static void handleStoreListRequest(std::shared_ptr<ClientSession> session, const std::string &jsonBody);
    static void handleStoreDetailReq(std::shared_ptr<ClientSession> session, const std::string &jsonBody);
    static void handleStoreInfoUpdateReq(std::shared_ptr<ClientSession> session, const std::string &jsonBody);
};