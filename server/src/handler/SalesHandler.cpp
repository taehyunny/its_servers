#include "SalesHandler.h"
#include "DbManager.h"
#include "Global_protocol.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <mariadb/conncpp.hpp>

using nlohmann::json;

// =========================================================
// 매출 통계 조회 (REQ_SALES_STAT = 3090)
// =========================================================
// 요청 body:
//   { "storeId": 1 }
//
// 응답 body:
//   {
//     "status":     200,
//     "totalSales": 10000000  <- STORES.total_sales 값
//   }
// =========================================================
void SalesHandler::handleSalesStat(
    std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    json req = json::parse(jsonBody);
    json res;
    try
    {
        int storeId = req.value("storeId", 0);
        if (storeId == 0)
        {
            res["status"] = 400;
            res["message"] = "storeId가 없습니다.";
            session->sendPacket(static_cast<uint16_t>(CmdID::RES_SALES_STAT), res);
            return;
        }

        auto conn = DBManager::getInstance().getConnection();

        // STORES.total_sales 조회
        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
            "SELECT total_sales FROM STORES WHERE store_id = ?"));
        pstmt->setInt(1, storeId);
        std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());

        if (rs->next())
        {
            res["status"] = 200;
            res["message"] = "조회 성공";
            res["totalSales"] = rs->getInt("total_sales");
        }
        else
        {
            res["status"] = 404;
            res["message"] = "해당 매장을 찾을 수 없습니다.";
        }

        session->sendPacket(static_cast<uint16_t>(CmdID::RES_SALES_STAT), res);
    }
    catch (const std::exception &e)
    {
        std::cerr << "[SalesHandler] handleSalesStat 오류: " << e.what() << std::endl;
        res["status"] = 500;
        res["message"] = "서버 내부 오류";
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_SALES_STAT), res);
    }
}
