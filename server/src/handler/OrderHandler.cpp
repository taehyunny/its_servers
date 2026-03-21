#include "OrderHandler.h"
#include "OrderDAO.h"
#include "AllDTOs.h"
#include "Global_protocol.h"
#include "ClientSession.h"
#include <iostream>

void OrderHandler::handleOrderCreate(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        // 1. JSON 까보기
        auto req = nlohmann::json::parse(jsonBody).get<OrderCreateReqDTO>();
        std::cout << "[OrderHandler] 🛒 유저 '" << req.userId << "'의 장바구니 결제 요청 수신 (총액: " << req.totalPrice << "원)" << std::endl;

        // 2. 태현님의 완벽한 트랜잭션 DAO 호출!
        OrderResult dbResult = OrderDAO::getInstance().createOrder(req);

        // 3. 응답 봉투(DTO) 준비
        OrderCreateResDTO res;
        res.status = dbResult.isSuccess ? 0 : 1; // 0: 성공, 1: 실패
        res.message = dbResult.message;
        res.orderId = dbResult.orderId; // 예: ORD-1710992345678

        // 4. 2021번으로 시원하게 발사!
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ORDER_CREATE), res);
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [OrderHandler] JSON 파싱 또는 처리 중 에러: " << e.what() << std::endl;

        OrderCreateResDTO errorRes;
        errorRes.status = 1;
        errorRes.message = "잘못된 주문 데이터 형식입니다.";
        errorRes.orderId = "";
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ORDER_CREATE), errorRes);
    }
}