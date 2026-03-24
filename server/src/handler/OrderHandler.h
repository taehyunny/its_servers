#pragma once
#include <memory>
#include <string>

class ClientSession; // 전방 선언

class OrderHandler
{
public:
    // 🚀 장바구니 결제 요청 처리기
    static void handleOrderCreate(std::shared_ptr<ClientSession> session, const std::string &jsonBody);
    static void handleOrderAccept(std::shared_ptr<ClientSession> session, const std::string &jsonBody); // 주문 수락 및 조리 시작 요청 (3000)
    static void handleCheckoutInfo(std::shared_ptr<ClientSession> session, const std::string &jsonBody);
};