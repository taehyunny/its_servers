#pragma once
#include "json.hpp"
#include "ClientSession.h"

class MenuHandler
{
public:
    // 앞서 경험한 대로 스마트 포인터 대신 일반 포인터(*)를 사용합니다.
    static void handleMenuListRequest(std::shared_ptr<ClientSession> session, const std::string &jsonBody);
    static void handleMenuEdit(std::shared_ptr<ClientSession> session, const std::string &jsonBody);
    static void handleMenuSoldOut(std::shared_ptr<ClientSession> session, const std::string &jsonBody);
    static void handleMenuOption(std::shared_ptr<ClientSession> session, const std::string &jsonBody);
};