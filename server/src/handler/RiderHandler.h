#pragma once
#include <string>
#include <memory>
#include "ClientSession.h"

class RiderHandler
{
public:
    // 배달 가능 주문 목록 조회 (4020)
    static void handleRiderOrderList(std::shared_ptr<ClientSession> session, const std::string &jsonBody);

    // 배달 완료 처리 (4010)
    static void handleDeliveryComplete(std::shared_ptr<ClientSession> session, const std::string &jsonBody);

    static void handlePickup(std::shared_ptr<ClientSession> session, const std::string &jsonBody);
};