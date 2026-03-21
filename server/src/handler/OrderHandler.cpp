#include "OrderHandler.h"
#include "OrderDAO.h"
#include "AllDTOs.h"
#include "Global_protocol.h"
#include "ClientSession.h"
#include "SessionManager.h"
#include <iostream>

// =========================================================================
// 🛒 1. 고객의 주문 생성 요청 (2020번) - 아까 만든 진짜 Create 로직!
// =========================================================================
void OrderHandler::handleOrderCreate(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        auto req = nlohmann::json::parse(jsonBody).get<OrderCreateReqDTO>();
        std::cout << "[OrderHandler] 🛒 유저 '" << req.userId << "'의 장바구니 결제 요청 수신" << std::endl;

        OrderResult dbResult = OrderDAO::getInstance().createOrder(req);

        OrderCreateResDTO res;
        res.status = dbResult.isSuccess ? 0 : 1;
        res.message = dbResult.message;
        res.orderId = dbResult.orderId;

        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ORDER_CREATE), res);
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [OrderHandler] 주문 생성 중 에러: " << e.what() << std::endl;
    }
}

// =========================================================================
// 🧑‍🍳 2. 사장님의 주문 수락 요청 (3000번) - 이름을 Accept로 제대로 바꿈!
// =========================================================================
void OrderHandler::handleOrderAccept(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        auto req = nlohmann::json::parse(jsonBody).get<OrderAcceptReqDTO>();
        std::cout << "[OrderHandler] 🧑‍🍳 사장님이 주문(" << req.orderId << ")을 수락. 예상시간: " << req.estimatedTime << "분" << std::endl;

        bool isDbSuccess = OrderDAO::getInstance().updateOrderStatus(req.orderId, 1);

        if (isDbSuccess)
        {
            // 액션 1: 사장님 응답
            OrderAcceptResDTO res = {0, "주문이 성공적으로 수락되었습니다."};
            session->sendPacket(static_cast<uint16_t>(CmdID::RES_ORDER_ACCEPT), res);

            // 액션 2: 고객 푸시 알림
            NotifyOrderStateDTO notifyCustomer;
            notifyCustomer.orderId = req.orderId;
            notifyCustomer.state = 1;
            notifyCustomer.message = "사장님이 조리를 시작했습니다! (" + std::to_string(req.estimatedTime) + "분 소요 예정)";

            std::string customerId = OrderDAO::getInstance().getCustomerIdByOrderId(req.orderId);
            SessionManager::getInstance().sendToUser(customerId, static_cast<uint16_t>(CmdID::NOTIFY_ORDER_STATE), notifyCustomer);

            // 액션 3: 라이더 브로드캐스트
            NotifyDeliveryCallDTO notifyRiders;
            notifyRiders.orderId = req.orderId;
            notifyRiders.pickupAddress = "황궁짜장 (송정점)";
            notifyRiders.deliveryAddress = "고객 배달 주소";

            int ROLE_RIDER = 2;
            SessionManager::getInstance().broadcastToRole(ROLE_RIDER, static_cast<uint16_t>(CmdID::NOTIFY_DELIVERY_CALL), notifyRiders);
        }
        else
        {
            OrderAcceptResDTO res = {1, "DB 오류로 주문 수락에 실패했습니다."};
            session->sendPacket(static_cast<uint16_t>(CmdID::RES_ORDER_ACCEPT), res);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [OrderHandler] 주문 수락 중 에러: " << e.what() << std::endl;
    }
}