#pragma once
#include <memory>
#include "json.hpp"
#include "ClientSession.h"

class StoreHandler
{
public:
    static void handleStoreListRequest(std::shared_ptr<ClientSession> session, const nlohmann::json &requestJson);
};