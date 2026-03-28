#include "SystemHandler.h"
#include "ClientSession.h"
#include "DbManager.h"
#include "json.hpp"
#include "Global_protocol.h"
#include <iostream>
#include <stdexcept>

void SystemHandler::handleHeartbeat(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        auto conn = DBManager::getInstance().getConnection();
        nlohmann::json res;

        // 🚀 1. 기본 하트비트 생존 응답 세팅
        res["status"] = 0;
        res["message"] = "Pong & Refresh Top Stores Only";

        // ---------------------------------------------------------
        // 🚀 정적 데이터(카테고리) 제거 완료! 오직 동적 데이터만 전송
        // ---------------------------------------------------------

        // 🚀 2. 매출 1등 매장 (UI 동기화용 풀세트)
        std::unique_ptr<sql::PreparedStatement> pstmtStore(conn->prepareStatement(
            "SELECT store_id, store_name, category, total_sales, rating, delivery_fee, " // 👈 category 추가
            "min_order_amount, review_count, delivery_time_range, icon_name "
            "FROM STORES ORDER BY total_sales DESC LIMIT 10"));
        std::unique_ptr<sql::ResultSet> rsStore(pstmtStore->executeQuery());

        res["topStores"] = nlohmann::json::array();
        while (rsStore->next())
        {
            res["topStores"].push_back({{"storeId", rsStore->getInt("store_id")},
                                        {"storeName", std::string(rsStore->getString("store_name"))},
                                        {"categoryName", std::string(rsStore->getString("category"))}, // 👈 프론트가 요청한 변수 추가!
                                        {"totalSales", rsStore->getInt("total_sales")},
                                        {"rating", rsStore->getDouble("rating")},
                                        {"deliveryFee", rsStore->getInt("delivery_fee")},
                                        {"minOrderAmount", rsStore->getInt("min_order_amount")},
                                        {"reviewCount", rsStore->getInt("review_count")},
                                        {"deliveryTimeRange", std::string(rsStore->getString("delivery_time_range"))},
                                        {"iconName", std::string(rsStore->getString("icon_name"))}});
        }

        // 🚀 3. 초경량화된 하트비트 응답 전송!
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_HEARTBEAT), res);
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [SystemHandler] 하트비트 에러: " << e.what() << std::endl;
        nlohmann::json errRes;
        errRes["status"] = 1;
        errRes["message"] = "Pong (Data Error)";
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_HEARTBEAT), errRes);
    }
}