#include "SystemHandler.h"
#include "ClientSession.h"
#include "DbManager.h"
#include "json.hpp"
#include "Global_protocol.h" // 프로토콜 ID가 정의된 헤더 (프로젝트 환경에 맞게 수정하세요!)
#include <iostream>
#include <stdexcept>

void SystemHandler::handleHeartbeat(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        // 💡 하트비트 로그가 너무 많이 찍혀서 콘솔이 지저분해지면 아래 줄은 주석 처리하세요!
        // std::cout << "[SystemHandler] 💓 하트비트(1000) 수신 -> 메인 데이터 갱신 시작" << std::endl;

        auto conn = DBManager::getInstance().getConnection();
        nlohmann::json res;

        // 🚀 1. 기본 하트비트 생존 응답 세팅
        res["status"] = 0;
        res["message"] = "Pong & Refresh Data";

        // 🚀 2. 카테고리 정보 긁어오기 (기존 로그인 시 주던 정보)
        std::unique_ptr<sql::PreparedStatement> pstmtCat(conn->prepareStatement(
            "SELECT category_id, name FROM CATEGORIES ORDER BY category_id ASC"));
        std::unique_ptr<sql::ResultSet> rsCat(pstmtCat->executeQuery());

        res["categories"] = nlohmann::json::array();
        while (rsCat->next())
        {
            res["categories"].push_back({{"categoryId", rsCat->getInt("category_id")},
                                         // 문자열 깨짐 방지를 위해 std::string으로 감싸기
                                         {"name", std::string(rsCat->getString("name"))}});
        }

        // 🚀 3. 매출 1등 매장 긁어오기 (total_sales 기준 내림차순 10개!)
        // 황궁쟁반의 순위가 바뀌면 여기서 바로 반영됩니다.
        std::unique_ptr<sql::PreparedStatement> pstmtStore(conn->prepareStatement(
            "SELECT store_id, store_name, total_sales, rating "
            "FROM STORES ORDER BY total_sales DESC LIMIT 10"));
        std::unique_ptr<sql::ResultSet> rsStore(pstmtStore->executeQuery());

        res["topStores"] = nlohmann::json::array();
        while (rsStore->next())
        {
            res["topStores"].push_back({{"storeId", rsStore->getInt("store_id")},
                                        {"storeName", std::string(rsStore->getString("store_name"))},
                                        {"totalSales", rsStore->getInt("total_sales")},
                                        {"rating", rsStore->getDouble("rating")}});
        }

        // 🚀 4. 하트비트 응답(1001번)으로 꽉 채운 데이터 전송!
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_HEARTBEAT), res);
    }
    catch (const std::exception &e)
    {
        // 에러가 나더라도 하트비트 핑퐁은 끊기면 안 됩니다! (클라이언트 튕김 방지)
        std::cerr << "🚨 [SystemHandler] 하트비트/새로고침 에러: " << e.what() << std::endl;

        nlohmann::json errRes;
        errRes["status"] = 1;
        errRes["message"] = "Pong (Data Error)";
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_HEARTBEAT), errRes);
    }
}