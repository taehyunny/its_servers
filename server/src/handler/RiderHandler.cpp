#include "RiderHandler.h"
#include "DbManager.h"
#include "SessionManager.h"
#include "Global_protocol.h"
#include "AllDTOs.h"
#include "OrderDAO.h"
#include "StoreDAO.h"
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

void RiderHandler::handleRiderOrderList(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    json res;
    try
    {
        auto conn = DBManager::getInstance().getConnection();

        // 조리중(1) 주문 조회
        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
            "SELECT order_id, total_price, delivery_address, created_at "
            "FROM ORDERS WHERE order_status = 1 ORDER BY created_at DESC"));

        std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());
        json orders = json::array();

        while (rs->next())
        {
            json order;
            std::string orderId = rs->getString("order_id").c_str();
            order["orderId"] = orderId;
            order["totalPrice"] = rs->getInt("total_price");
            order["deliveryAddress"] = rs->getString("delivery_address").c_str();
            order["createdAt"] = rs->getString("created_at").c_str();

            // 메뉴 요약(menuSummary) 만들기
            std::unique_ptr<sql::PreparedStatement> pstmtItems(conn->prepareStatement(
                "SELECT M.menu_name FROM ORDER_ITEMS OI "
                "JOIN MENUS M ON OI.menu_id = M.menu_id "
                "WHERE OI.order_id = ? LIMIT 1")); // 첫 번째 메뉴만 가져옴
            pstmtItems->setString(1, orderId);
            std::unique_ptr<sql::ResultSet> rsItems(pstmtItems->executeQuery());

            std::unique_ptr<sql::PreparedStatement> pstmtCount(conn->prepareStatement(
                "SELECT COUNT(*) as cnt FROM ORDER_ITEMS WHERE order_id = ?"));
            pstmtCount->setString(1, orderId);
            std::unique_ptr<sql::ResultSet> rsCount(pstmtCount->executeQuery());

            std::string menuSummary = "메뉴";
            if (rsItems->next())
            {
                menuSummary = rsItems->getString("menu_name").c_str();
                if (rsCount->next())
                {
                    int cnt = rsCount->getInt("cnt");
                    if (cnt > 1)
                    {
                        menuSummary += " 외 " + std::to_string(cnt - 1) + "건";
                    }
                }
            }
            order["menuSummary"] = menuSummary;
            orders.push_back(order);
        }

        res["status"] = 200;
        res["message"] = "조회 성공";
        res["orders"] = orders;
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_RIDER_ORDER_LIST), res);
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [RiderHandler] 콜 리스트 조회 오류: " << e.what() << std::endl;
        res["status"] = 500;
        res["message"] = "서버 내부 오류";
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_RIDER_ORDER_LIST), res);
    }
}

void RiderHandler::handleDeliveryComplete(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    json res;
    try
    {
        json req = json::parse(jsonBody);
        std::string orderId = req.value("orderId", "");

        if (orderId.empty())
        {
            res["status"] = 400;
            res["message"] = "orderId가 없습니다.";
            session->sendPacket(static_cast<uint16_t>(CmdID::RES_DELIVERY_COMPLETE), res);
            return;
        }

        auto conn = DBManager::getInstance().getConnection();

        // 🚀 order_status = 4 (배달완료) 로 UPDATE
        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
            "UPDATE ORDERS SET order_status = 4 WHERE order_id = ?"));
        pstmt->setString(1, orderId);
        int affected = pstmt->executeUpdate();

        if (affected > 0)
        {
            res["status"] = 200;
            res["message"] = "배달 완료 처리되었습니다.";
            res["orderId"] = orderId;

            // 🚀 고객에게 "배달 완료!" 푸시 알림 전송 (9010)
            // 고객 ID 조회를 위해 OrderDAO 사용 (이미 OrderHandler 등에서 쓰고 계신 로직 적용)
            std::unique_ptr<sql::PreparedStatement> pstmtUser(conn->prepareStatement(
                "SELECT user_id FROM ORDERS WHERE order_id = ?"));
            pstmtUser->setString(1, orderId);
            std::unique_ptr<sql::ResultSet> rsUser(pstmtUser->executeQuery());

            if (rsUser->next())
            {
                std::string customerId = rsUser->getString("user_id").c_str();
                NotifyOrderStateDTO notifyCustomer = {orderId, 4, "배달이 완료되었습니다. 맛있게 드세요! 😋"};
                SessionManager::getInstance().sendToUser(customerId, static_cast<uint16_t>(CmdID::NOTIFY_ORDER_STATE), notifyCustomer);
            }
        }
        else
        {
            res["status"] = 404;
            res["message"] = "해당 주문을 찾을 수 없습니다.";
        }

        session->sendPacket(static_cast<uint16_t>(CmdID::RES_DELIVERY_COMPLETE), res);
        std::cout << "[RiderHandler] ✅ 배달 완료 처리: " << orderId << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [RiderHandler] 배달 완료 처리 오류: " << e.what() << std::endl;
        res["status"] = 500;
        res["message"] = "서버 내부 오류";
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_DELIVERY_COMPLETE), res);
    }
}

void RiderHandler::handlePickup(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        auto req = nlohmann::json::parse(jsonBody);
        std::string orderId = req.value("orderId", "");

        auto conn = DBManager::getInstance().getConnection();
        // 1. 상태 업데이트 (3: 배달중)
        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
            "UPDATE ORDERS SET order_status = 3 WHERE order_id = ?"));
        pstmt->setString(1, orderId);
        pstmt->executeUpdate();

        // 2. 라이더에게 응답 (4031)
        nlohmann::json res = {{"status", 200}, {"orderId", orderId}};
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_PICKUP), res);

        // 3. 사장님에게 알림 (9010) - 포스기 상태 업데이트용
        int storeId = OrderDAO::getInstance().getStoreIdByOrderId(orderId);
        std::string ownerId = StoreDAO::getInstance().getOwnerIdByStoreId(storeId);
        if (!ownerId.empty())
        {
            nlohmann::json notify = {
                {"orderId", orderId},
                {"state", 3},
                {"message", "라이더가 음식을 픽업했습니다. 배달을 시작합니다!"}};
            SessionManager::getInstance().sendToUser(ownerId, static_cast<uint16_t>(CmdID::NOTIFY_ORDER_STATE), notify);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [RiderHandler] 배달 픽업 처리 오류: " << e.what() << std::endl;
    }
}