#include "AdminHandler.h"
#include "AdminDAO.h"
#include "Global_protocol.h"
#include "AllDTOs.h"
#include "ClientSession.h"
#include "SessionManager.h"
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// 🚀 [신규] 관리자 백그라운드 자동 인증 로직
void AdminHandler::handleAdminInit(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        auto req = nlohmann::json::parse(jsonBody);

        // 1. 프론트엔드가 보내준 규격에 맞춰서 값 추출! (로그: {"adminId":"admin", "role":3})
        std::string adminId = req.value("adminId", "admin");
        int role = req.value("role", 3);

        /* 💡 (선택) 나중에 보안을 강화하고 싶다면 프론트와 합의 후 주석을 푸세요.
        std::string secretKey = req.value("secretKey", "");
        if (secretKey != "eats_admin_secret_2026!") {
            std::cerr << "🚨 [AdminHandler] 관리자 인증 실패: 잘못된 시크릿 키 접근!" << std::endl;
            return;
        }
        */

        // 2. ✅ 세션에 "관리자" 명찰 달아주기 (ClientSession의 authenticate 활용)
        session->authenticate(adminId, role);

        // 3. 🚀 [가장 중요] SessionManager의 userMap에 공식 등록!
        // 이 코드가 있어야 나중에 관리자에게 9040번 푸시를 정확하게 쏠 수 있습니다.
        SessionManager::getInstance().registerUser(adminId, role, session);
        std::cout << "\n==================================================" << std::endl;
        std::cout << "[AdminHandler] 👑 백그라운드 관리자 인증 완료!" << std::endl;
        std::cout << " >>> [DEBUG] 등록된 ID: " << adminId << ", Role: " << role << " (FD: " << session->getFd() << ")" << std::endl;
        std::cout << " >>> [DEBUG] 해당 소켓이 9040번(문의 푸시) 수신 대기 상태가 되었습니다." << std::endl;
        std::cout << "==================================================\n"
                  << std::endl;

        // 4. 프론트엔드에게 성공 응답 전송 (5091)
        nlohmann::json res;
        res["status"] = 200;
        res["message"] = "관리자 권한이 부여되었습니다.";
        session->sendPacket(static_cast<uint16_t>(5091), res); // CmdID::RES_ADMIN_INIT
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [AdminHandler] 관리자 초기화 에러: " << e.what() << std::endl;
    }
}

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