#pragma once
#include <memory>
#include <string>
#include "ClientSession.h"

class AdminHandler
{
public:
    // 👑 관리자 전체 주문 내역 검색 (REQ_ADMIN_ORDER_LIST = 5020)
    static void handleAdminOrderList(std::shared_ptr<ClientSession> session, const std::string &jsonBody);
    static void handleAdminInit(std::shared_ptr<ClientSession> session, const std::string &jsonBody);
};