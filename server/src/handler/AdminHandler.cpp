#include "AdminHandler.h"
#include "AdminDAO.h"
#include "Global_protocol.h"
#include "AllDTOs.h"
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

void AdminHandler::handleAdminOrderList(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        // 1. 클라이언트 요청 데이터 파싱
        json reqJson = json::parse(jsonBody);
        auto req = reqJson.get<ReqAdminOrderListDTO>();

        std::cout << "\n[AdminHandler] 👑 관리자 주문 검색 요청" << std::endl;
        std::cout << " ┣ 검색 타입: " << (req.searchType.empty() ? "전체조회" : req.searchType) << std::endl;
        std::cout << " ┗ 검색 키워드: " << (req.keyword.empty() ? "(없음)" : req.keyword) << std::endl;

        // 2. 🚀 AdminDAO를 통해 신(God)의 시야로 전체 데이터 스캔!
        std::vector<AdminOrderDTO> orders = AdminDAO::getInstance().searchOrders(req.searchType, req.keyword);

        // 3. 성공 응답 DTO 조립
        ResAdminOrderListDTO res;
        res.status = 200;
        res.orders = orders;

        // 4. 클라이언트에게 5021번 패킷 전송
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ADMIN_ORDER_LIST), json(res));

        std::cout << "[AdminHandler] ✅ 관리자 주문 검색 완료 (" << orders.size() << "건 반환)\n"
                  << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [AdminHandler] 관리자 주문 검색 에러: " << e.what() << std::endl;

        // 에러 발생 시 status 500과 빈 배열을 전송하여 클라이언트 앱이 뻗는 것을 방지
        ResAdminOrderListDTO res;
        res.status = 500;
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ADMIN_ORDER_LIST), json(res));
    }
}