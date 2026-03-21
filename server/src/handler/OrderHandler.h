#pragma once
#include <memory>
#include <string>

class ClientSession; // 전방 선언

class OrderHandler
{
public:
    // 🚀 장바구니 결제 요청 처리기
    static void handleOrderCreate(std::shared_ptr<ClientSession> session, const std::string &jsonBody);
};