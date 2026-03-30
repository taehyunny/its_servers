#include "OrderHandler.h"
#include "OrderDAO.h"
#include "MenuDAO.h" // 🚀 서버 단독 검증을 위해 추가!
#include "AllDTOs.h"
#include "Global_protocol.h"
#include "ClientSession.h"
#include "SessionManager.h"
#include "DbManager.h" // DB 연결을 위한 매니저 클래스
#include "StoreDAO.h"
#include "UserDAO.h"
#include <iostream>
#include <mariadb/conncpp.hpp>
#include <nlohmann/json.hpp>
#include <ctime>
#include <iomanip>
#include <regex> // 🚀 숫자 추출을 위해 추가

using nlohmann::json;

// =========================================================================
// 🧑‍🍳 2. 사장님의 주문 수락 요청 (3000번)
// =========================================================================
void OrderHandler::handleOrderAccept(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    auto conn = DBManager::getInstance().getConnection();
    try
    {
        auto req = nlohmann::json::parse(jsonBody).get<OrderAcceptReqDTO>();
        if (req.orderId.empty())
            throw std::runtime_error("주문 번호가 누락되었습니다.");

        req.orderId.erase(0, req.orderId.find_first_not_of(" \t\r\n"));
        req.orderId.erase(req.orderId.find_last_not_of(" \t\r\n") + 1);
        std::cout << "[OrderHandler] 🧑🍳 정제된 ID로 수락 시도: [" << req.orderId << "]" << std::endl;

        conn->setAutoCommit(false);

        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
            "UPDATE ORDERS SET order_status = 1 WHERE order_id = ? AND order_status = 0"));
        pstmt->setString(1, req.orderId);
        int affected = pstmt->executeUpdate();

        if (affected == 0)
        {
            throw std::runtime_error("이미 처리되었거나 존재하지 않는 주문입니다.");
        }

        // 매출 누적
        std::unique_ptr<sql::PreparedStatement> pstmtSales(conn->prepareStatement(
            "UPDATE STORES S JOIN ORDERS O ON O.store_id = S.store_id "
            "SET S.total_sales = S.total_sales + O.total_price WHERE O.order_id = ?"));
        pstmtSales->setString(1, req.orderId);
        pstmtSales->executeUpdate();

        conn->commit();
        conn->setAutoCommit(true);

        // 사장님 응답
        OrderAcceptResDTO res = {200, "주문 수락이 완료되었습니다."};
        res.orderId = req.orderId;
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ORDER_ACCEPT), nlohmann::json(res));

        // 🚀 고객 푸시 (state = 1)
        std::string customerId = OrderDAO::getInstance().getCustomerIdByOrderId(req.orderId);
        if (!customerId.empty())
        {
            NotifyOrderStateDTO notify;
            notify.orderId = req.orderId;
            notify.state = 1;
            notify.message = "사장님이 조리를 시작했습니다! (" + std::to_string(req.estimatedTime) + "분 소요 예정)";
            SessionManager::getInstance().sendToUser(customerId, static_cast<uint16_t>(CmdID::NOTIFY_ORDER_STATE), nlohmann::json(notify));
        }

        std::cout << "[OrderHandler] ✅ 주문 수락 처리 완벽 성공!" << std::endl;
    }
    catch (const std::exception &e)
    {
        if (conn)
        {
            conn->rollback();
            conn->setAutoCommit(true);
        }
        std::cerr << "🚨 [OrderHandler] 주문 수락 실패: " << e.what() << std::endl;
        OrderAcceptResDTO res = {500, std::string(e.what())};
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ORDER_ACCEPT), nlohmann::json(res));
    }
}

void OrderHandler::handleChangeOrderState(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    auto conn = DBManager::getInstance().getConnection();
    try
    {
        auto req = nlohmann::json::parse(jsonBody).get<ReqChangeOrderStateDTO>();
        std::cout << "[OrderHandler] 🔄 주문 상태 변경 요청 (ID: " << req.orderId << ", NewState: " << req.newState << ")" << std::endl;

        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
            "UPDATE ORDERS SET order_status = ? WHERE order_id = ?"));
        pstmt->setInt(1, req.newState);
        pstmt->setString(2, req.orderId);
        int affected = pstmt->executeUpdate();

        if (affected > 0)
        {
            ResChangeOrderStateDTO res = {200, "주문 상태가 성공적으로 변경되었습니다."};
            session->sendPacket(static_cast<uint16_t>(CmdID::RES_CHANGE_ORDER_STATE), nlohmann::json(res));

            std::string customerId = OrderDAO::getInstance().getCustomerIdByOrderId(req.orderId);

            if (!customerId.empty())
            {
                NotifyOrderStateDTO notifyCustomer;
                notifyCustomer.orderId = req.orderId;
                notifyCustomer.state = req.newState;

                if (req.newState == 2)
                {
                    notifyCustomer.message = "주문하신 음식의 조리가 완료되었습니다! 🍳";
                }
                else if (req.newState == 3)
                {
                    notifyCustomer.message = "라이더님이 음식을 픽업하여 배달을 시작했습니다! 🛵💨";
                }
                else if (req.newState == 4)
                {
                    notifyCustomer.message = "배달이 완료되었습니다. 맛있게 드세요! 😋";
                }
                else
                {
                    notifyCustomer.message = "주문 상태가 변경되었습니다.";
                }

                SessionManager::getInstance().sendToUser(
                    customerId,
                    static_cast<uint16_t>(CmdID::NOTIFY_ORDER_STATE),
                    nlohmann::json(notifyCustomer));
            }
            std::cout << "[OrderHandler] ✅ 상태 변경 및 고객 알림 완벽 라우팅 완료" << std::endl;
        }
        else
        {
            throw std::runtime_error("주문을 찾을 수 없거나 DB 업데이트에 실패했습니다.");
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [OrderHandler] 상태 변경 중 에러: " << e.what() << std::endl;
        ResChangeOrderStateDTO res = {500, "상태 변경 중 서버 오류가 발생했습니다."};
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_CHANGE_ORDER_STATE), nlohmann::json(res));
    }
}

void OrderHandler::handleOrderList(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
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
            session->sendPacket(static_cast<uint16_t>(CmdID::RES_ORDER_LIST), res);
            return;
        }

        auto conn = DBManager::getInstance().getConnection();

        // 🚀 1. 쿼리 수정: reject_reason 컬럼 추가 & 상태 4(완료), 9(취소/거절) 포함!
        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
            "SELECT order_id, total_price, order_status, "
            "delivery_address, created_at, store_request, rider_request, reject_reason "
            "FROM ORDERS "
            "WHERE store_id = ? "
            "AND order_status IN (0, 1, 2, 3, 4, 9) "
            "ORDER BY created_at DESC"));
        pstmt->setInt(1, storeId);
        std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());

        json orders = json::array();

        while (rs->next())
        {
            json order;
            std::string orderId = rs->getString("order_id").c_str();

            order["orderId"] = orderId;
            order["totalPrice"] = rs->getInt("total_price");
            order["state"] = rs->getInt("order_status");
            order["deliveryAddress"] = rs->getString("delivery_address").c_str();
            order["createdAt"] = rs->getString("created_at").c_str();

            order["storeRequest"] = rs->isNull("store_request") ? "" : rs->getString("store_request").c_str();
            order["riderRequest"] = rs->isNull("rider_request") ? "" : rs->getString("rider_request").c_str();

            // 🚀 2. 거절 사유 파싱: DB에 값이 있으면 넣고 없으면 빈 문자열("") 세팅
            order["rejectReason"] = rs->isNull("reject_reason") ? "" : rs->getString("reject_reason").c_str();

            // --- 메뉴 상세(ORDER_DETAILS) 조회 시작 ---
            std::unique_ptr<sql::PreparedStatement> pstmtItems(conn->prepareStatement(
                "SELECT menu_id, menu_name, quantity, "
                "price AS unit_price, selected_options "
                "FROM ORDER_DETAILS "
                "WHERE order_id = ?"));
            pstmtItems->setString(1, orderId);
            std::unique_ptr<sql::ResultSet> rsItems(pstmtItems->executeQuery());

            json items = json::array();
            std::string firstMenuName = "";
            int itemCount = 0;

            while (rsItems->next())
            {
                json item;
                item["menuId"] = rsItems->getInt("menu_id");
                item["menuName"] = rsItems->getString("menu_name").c_str();
                item["quantity"] = rsItems->getInt("quantity");
                item["unitPrice"] = rsItems->getInt("unit_price");

                std::string optStr = rsItems->isNull("selected_options") ? "[]" : rsItems->getString("selected_options").c_str();
                try
                {
                    item["options"] = json::parse(optStr);
                    if (!item["options"].is_array())
                    {
                        item["options"] = json::array();
                    }
                }
                catch (...)
                {
                    item["options"] = json::array();
                }

                if (itemCount == 0)
                    firstMenuName = rsItems->getString("menu_name").c_str();
                itemCount++;
                items.push_back(item);
            }

            std::string menuSummary = firstMenuName;
            if (itemCount > 1)
                menuSummary += " 외 " + std::to_string(itemCount - 1) + "건";

            order["menuSummary"] = menuSummary;
            order["items"] = items;

            orders.push_back(order);
        }

        res["status"] = 200;
        res["message"] = "조회 성공";
        res["orders"] = orders;

        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ORDER_LIST), res);
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [OrderHandler] handleOrderList 오류: " << e.what() << std::endl;
        res["status"] = 500;
        res["message"] = "서버 내부 오류";
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ORDER_LIST), res);
    }
}

// 🚀 1. 일반 주문 내역 조회 (최근 주문 순)
void OrderHandler::handleOrderHistory(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        auto req = nlohmann::json::parse(jsonBody).get<ReqOrderHistoryDTO>();
        std::cout << "[OrderHandler] 📜 주문 내역 조회 요청 (UserID: " << req.userId << ")" << std::endl;

        // 💡 DAO가 데이터를 완벽하게 긁어온다고 가정!
        std::vector<OrderHistoryItemDTO> historyList = OrderDAO::getInstance().getOrderHistory(req.userId, "");

        // 🚀 매의 눈 수정: 200 대신 DTO 규격에 맞게 0(성공)으로 변경!
        ResOrderHistoryDTO res = {0, historyList};
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ORDER_HISTORY), nlohmann::json(res));

        std::cout << "[OrderHandler] ✅ 주문 내역 " << historyList.size() << "건 전송 완료" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [OrderHandler] 주문 내역 조회 에러: " << e.what() << std::endl;
        // 🚀 매의 눈 수정: 500 대신 DTO 규격에 맞게 1(실패)로 변경!
        ResOrderHistoryDTO res = {1, {}};
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ORDER_HISTORY), nlohmann::json(res));
    }
}

// 🚀 2. 검색어가 포함된 주문 내역 조회
void OrderHandler::handleOrderHistorySearch(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        auto req = nlohmann::json::parse(jsonBody).get<ReqOrderHistorySearchDTO>();
        std::cout << "[OrderHandler] 🔍 주문 내역 검색 요청 (UserID: " << req.userId << ", Keyword: " << req.keyword << ")" << std::endl;

        std::vector<OrderHistoryItemDTO> historyList = OrderDAO::getInstance().getOrderHistory(req.userId, req.keyword);

        ResOrderHistoryDTO res = {0, historyList};
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ORDER_HISTORY_SEARCH), nlohmann::json(res));
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [OrderHandler] 주문 내역 검색 에러: " << e.what() << std::endl;
        ResOrderHistoryDTO res = {500, {}};
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ORDER_HISTORY_SEARCH), nlohmann::json(res));
    }
}

void OrderHandler::handleCreateOrder(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    auto conn = DBManager::getInstance().getConnection();
    try
    {
        auto req = nlohmann::json::parse(jsonBody).get<OrderCreateReqDTO>();
        std::cout << "[OrderHandler] 📦 신규 주문 생성 요청 (UserID: " << req.userId << ", StoreID: " << req.storeId << ")" << std::endl;

        int itemSum = 0;
        for (const auto &item : req.items)
        {
            itemSum += (item.unitPrice * item.quantity);
        }

        int deliveryFee = StoreDAO::getInstance().getDeliveryFee(req.storeId);
        if (itemSum + deliveryFee != req.totalPrice)
        {
            std::cerr << "🚨 [OrderHandler] 금액 변조 의심! (계산됨: " << (itemSum + deliveryFee) << ", 수신됨: " << req.totalPrice << ")" << std::endl;
        }

        conn->setAutoCommit(false);

        time_t now = time(nullptr);
        char timeBuf[80];
        strftime(timeBuf, sizeof(timeBuf), "%Y%m%d%H%M%S", localtime(&now));
        int randomNum = rand() % 1000;
        char randomBuf[10];
        sprintf(randomBuf, "%03d", randomNum);

        std::string newOrderId = "ORD-" + std::string(timeBuf) + "-" + std::string(randomBuf) + "-" + req.userId;

        std::unique_ptr<sql::PreparedStatement> pstmtOrder(conn->prepareStatement(
            "INSERT INTO ORDERS (order_id, user_id, store_id, total_price, delivery_address, store_request, rider_request, order_status, created_at) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, 0, NOW())"));

        pstmtOrder->setString(1, newOrderId);
        pstmtOrder->setString(2, req.userId);
        pstmtOrder->setString(3, std::to_string(req.storeId));
        pstmtOrder->setInt(4, req.totalPrice);
        pstmtOrder->setString(5, req.deliveryAddress);
        pstmtOrder->setString(6, req.storeRequest);
        pstmtOrder->setString(7, req.riderRequest);
        pstmtOrder->executeUpdate();

        std::unique_ptr<sql::PreparedStatement> pstmtItems(conn->prepareStatement(
            "INSERT INTO ORDER_DETAILS (order_id, menu_id, menu_name, quantity, price, selected_options) VALUES (?, ?, ?, ?, ?, ?)"));

        nlohmann::json pushItemsArray = nlohmann::json::array();
        std::string firstMenuName = "";

        for (size_t i = 0; i < req.items.size(); ++i)
        {
            const auto &item = req.items[i];

            std::string menuName = MenuDAO::getInstance().getMenuName(item.menuId);
            if (menuName.empty())
                menuName = "메뉴 #" + std::to_string(item.menuId);
            if (i == 0)
                firstMenuName = menuName;

            nlohmann::json opts = item.selectedOptions;
            nlohmann::json optionNamesArray = nlohmann::json::array();

            if (opts.is_array())
            {
                for (const auto &optId : opts)
                {
                    if (optId.is_number_integer())
                    {
                        std::string optName = MenuDAO::getInstance().getOptionName(item.menuId, optId.get<int>());
                        if (!optName.empty())
                            optionNamesArray.push_back(optName);
                    }
                }
            }

            pstmtItems->setString(1, newOrderId);
            pstmtItems->setInt(2, item.menuId);
            pstmtItems->setString(3, menuName);
            pstmtItems->setInt(4, item.quantity);
            pstmtItems->setInt(5, item.unitPrice);
            pstmtItems->setString(6, optionNamesArray.dump());
            pstmtItems->executeUpdate();

            nlohmann::json pushItem;
            pushItem["menuId"] = item.menuId;
            pushItem["menuName"] = menuName;
            pushItem["quantity"] = item.quantity;
            pushItem["unitPrice"] = item.unitPrice;
            pushItem["options"] = optionNamesArray;

            pushItemsArray.push_back(pushItem);
        }

        conn->commit();
        conn->setAutoCommit(true);
        std::cout << "[OrderHandler] ✅ 주문 생성 및 DB 저장 완벽 성공! (OrderID: " << newOrderId << ")" << std::endl;

        OrderCreateResDTO res = {0, "주문이 성공적으로 생성되었습니다.", newOrderId};
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ORDER_CREATE), nlohmann::json(res));

        std::string ownerId = StoreDAO::getInstance().getOwnerIdByStoreId(req.storeId);
        if (!ownerId.empty())
        {
            nlohmann::json pushData;
            pushData["orderId"] = newOrderId;
            pushData["deliveryAddress"] = req.deliveryAddress;
            pushData["totalPrice"] = req.totalPrice;
            pushData["state"] = 0;

            char createdAtBuf[80];
            strftime(createdAtBuf, sizeof(createdAtBuf), "%Y-%m-%d %H:%M:%S", localtime(&now));
            pushData["createdAt"] = std::string(createdAtBuf);
            pushData["storeRequest"] = req.storeRequest;
            pushData["riderRequest"] = req.riderRequest;
            pushData["menuSummary"] = firstMenuName + (req.items.size() > 1 ? " 외 " + std::to_string(req.items.size() - 1) + "건" : "");
            pushData["items"] = pushItemsArray;

            SessionManager::getInstance().sendToUser(ownerId, static_cast<uint16_t>(CmdID::NOTIFY_NEW_ORDER), pushData);
        }
    }
    catch (const std::exception &e)
    {
        if (conn)
        {
            conn->rollback();
            conn->setAutoCommit(true);
        }
        std::cerr << "🚨 [OrderHandler] 주문 생성 실패: " << e.what() << std::endl;
        OrderCreateResDTO res = {1, std::string(e.what()), ""};
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ORDER_CREATE), nlohmann::json(res));
    }
}

// =========================================================
// 🧑‍🍳 조리 시간 재설정 및 전파 (REQ_COOK_TIME_SET = 3020)
// =========================================================
void OrderHandler::handleCookTimeSet(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        auto req = nlohmann::json::parse(jsonBody).get<ReqCookTimeSetDTO>();
        std::cout << "[OrderHandler] ⏱️ 조리 시간 설정 요청 (OrderID: " << req.orderId << ", " << req.cookTime << "분)" << std::endl;

        auto conn = DBManager::getInstance().getConnection();

        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
            "SELECT S.delivery_time_range, S.cook_time "
            "FROM STORES S "
            "JOIN ORDERS O ON S.store_id = O.store_id "
            "WHERE O.order_id = ?"));
        pstmt->setString(1, req.orderId);
        std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());

        int netDeliveryTime = 15;

        if (rs->next())
        {
            std::string timeRange = rs->isNull("delivery_time_range") ? "" : rs->getString("delivery_time_range").c_str();
            int defaultCookTime = rs->isNull("cook_time") ? 30 : rs->getInt("cook_time");

            std::regex re("\\d+");
            auto begin = std::sregex_iterator(timeRange.begin(), timeRange.end(), re);
            auto end = std::sregex_iterator();

            int maxTotalTime = 0;
            for (std::sregex_iterator i = begin; i != end; ++i)
            {
                int val = std::stoi(i->str());
                if (val > maxTotalTime)
                    maxTotalTime = val;
            }

            if (maxTotalTime > 0)
            {
                netDeliveryTime = maxTotalTime - defaultCookTime;
            }

            if (netDeliveryTime < 10)
            {
                netDeliveryTime = 15;
            }
        }

        int totalEstimatedTime = req.cookTime + netDeliveryTime;

        ResCookTimeSetDTO res = {200, "조리 시간이 성공적으로 설정되었습니다."};
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_COOK_TIME_SET), nlohmann::json(res));

        std::string customerId = OrderDAO::getInstance().getCustomerIdByOrderId(req.orderId);
        if (!customerId.empty())
        {
            NotifyOrderStateDTO notifyCustomer;
            notifyCustomer.orderId = req.orderId;
            notifyCustomer.state = 1;
            notifyCustomer.message = "사장님이 조리를 시작했습니다! (약 " + std::to_string(totalEstimatedTime) + "분 내 도착 예정 🛵)";

            SessionManager::getInstance().sendToUser(
                customerId,
                static_cast<uint16_t>(CmdID::NOTIFY_ORDER_STATE),
                nlohmann::json(notifyCustomer));
        }

        NotifyDeliveryCallDTO notifyRiders;
        notifyRiders.orderId = req.orderId;
        notifyRiders.pickupAddress = StoreDAO::getInstance().getStoreDetail(OrderDAO::getInstance().getStoreIdByOrderId(req.orderId)).storeAddress;
        notifyRiders.deliveryAddress = OrderDAO::getInstance().getDeliveryAddressByOrderId(req.orderId);
        notifyRiders.deliveryFee = 3500;
        notifyRiders.menuSummary = "조리 완료까지 " + std::to_string(req.cookTime) + "분 남음";

        int ROLE_RIDER = 2;
        SessionManager::getInstance().broadcastToRole(
            ROLE_RIDER,
            static_cast<uint16_t>(CmdID::NOTIFY_DELIVERY_CALL),
            nlohmann::json(notifyRiders));
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [OrderHandler] 조리 시간 설정 중 에러: " << e.what() << std::endl;
        ResCookTimeSetDTO res = {500, "서버 내부 오류로 설정에 실패했습니다."};
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_COOK_TIME_SET), nlohmann::json(res));
    }
}

void OrderHandler::handleDeliveryComplete(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    auto conn = DBManager::getInstance().getConnection();
    try
    {
        auto req = nlohmann::json::parse(jsonBody).get<ReqDeliveryCompleteDTO>();
        std::cout << "[OrderHandler] 🛵 라이더 배달 완료 보고 (ID: " << req.orderId << ")" << std::endl;

        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
            "UPDATE ORDERS SET order_status = 4 WHERE order_id = ?"));
        pstmt->setString(1, req.orderId);
        int affected = pstmt->executeUpdate();

        if (affected > 0)
        {
            ResDeliveryCompleteDTO res = {200, "배달 완료 처리가 정상적으로 기록되었습니다."};
            session->sendPacket(static_cast<uint16_t>(CmdID::RES_DELIVERY_COMPLETE), nlohmann::json(res));

            std::string customerId = OrderDAO::getInstance().getCustomerIdByOrderId(req.orderId);
            if (!customerId.empty())
            {
                NotifyOrderStateDTO notifyCustomer;
                notifyCustomer.orderId = req.orderId;
                notifyCustomer.state = 4;
                notifyCustomer.message = "배달이 완료되었습니다. 맛있게 드세요! 😋";

                SessionManager::getInstance().sendToUser(
                    customerId,
                    static_cast<uint16_t>(CmdID::NOTIFY_ORDER_STATE),
                    nlohmann::json(notifyCustomer));
            }

            int storeId = OrderDAO::getInstance().getStoreIdByOrderId(req.orderId);
            std::string ownerId = StoreDAO::getInstance().getOwnerIdByStoreId(storeId);
            if (!ownerId.empty())
            {
                NotifyOrderStateDTO notifyOwner;
                notifyOwner.orderId = req.orderId;
                notifyOwner.state = 4;
                notifyOwner.message = "주문 번호[" + req.orderId + "] 배달이 완료되었습니다.";

                SessionManager::getInstance().sendToUser(
                    ownerId,
                    static_cast<uint16_t>(CmdID::NOTIFY_ORDER_STATE),
                    nlohmann::json(notifyOwner));
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [OrderHandler] 배달 완료 처리 중 오류: " << e.what() << std::endl;
        ResDeliveryCompleteDTO res = {500, "서버 오류로 완료 처리에 실패했습니다."};
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_DELIVERY_COMPLETE), nlohmann::json(res));
    }
}

// 🚀 [완벽 수정본] 2027: 결제 화면 정보 응답 핸들러
void OrderHandler::handleCheckoutInfo(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        auto req = nlohmann::json::parse(jsonBody).get<ReqCheckoutInfoDTO>();
        ResCheckoutInfoDTO res = {};
        res.status = 200;

        auto conn = DBManager::getInstance().getConnection();

        // 🚀 1. 유저 정보 조회 (방어적 매핑)
        std::unique_ptr<sql::PreparedStatement> pstmtUser(conn->prepareStatement(
            "SELECT customer_grade, card_number, account_number, point, address "
            "FROM CUSTOMERS WHERE user_id = ?"));
        pstmtUser->setString(1, req.userId);
        std::unique_ptr<sql::ResultSet> rsUser(pstmtUser->executeQuery());

        if (rsUser->next())
        {
            res.customerGrade = rsUser->isNull("customer_grade") ? "일반" : std::string(rsUser->getString("customer_grade"));
            res.cardNumber = rsUser->isNull("card_number") ? "" : std::string(rsUser->getString("card_number"));
            res.accountNumber = rsUser->isNull("account_number") ? "" : std::string(rsUser->getString("account_number"));
            res.userPoint = rsUser->isNull("point") ? 0 : rsUser->getInt("point");
            res.userAddress = rsUser->isNull("address") ? "주소 미상" : std::string(rsUser->getString("address"));
        }

        // 🚀 2. 매장 정보 조회
        std::unique_ptr<sql::PreparedStatement> pstmtStore(conn->prepareStatement(
            "SELECT min_order_amount, delivery_fee, store_address, pickup_time "
            "FROM STORES WHERE store_id = ?"));
        pstmtStore->setInt(1, req.storeId);
        std::unique_ptr<sql::ResultSet> rsStore(pstmtStore->executeQuery());

        if (rsStore->next())
        {
            res.minOrderAmount = rsStore->getInt("min_order_amount");
            res.deliveryFee = rsStore->getInt("delivery_fee"); // 원본 배달비 보존
            res.storeAddress = rsStore->isNull("store_address") ? "" : std::string(rsStore->getString("store_address"));
            res.pickupTime = rsStore->isNull("pickup_time") ? "" : std::string(rsStore->getString("pickup_time"));
        }

        // 🚀 3. 비즈니스 로직: 와우 혜택 적용 (적용 배달비 분리)
        if (res.customerGrade == "와우")
        {
            res.appliedDeliveryFee = 0; // 프론트엔드 계산용 실제 청구 배달비
            std::cout << "[OrderHandler] 🎉 와우 회원(" << req.userId << ") 접속! 배달비 무료 혜택 적용." << std::endl;
        }
        else
        {
            res.appliedDeliveryFee = res.deliveryFee;
        }

        session->sendPacket(static_cast<uint16_t>(CmdID::RES_CHECKOUT_INFO), nlohmann::json(res));
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [OrderHandler] handleCheckoutInfo 오류: " << e.what() << std::endl;
        ResCheckoutInfoDTO errRes;
        errRes.status = 500;
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_CHECKOUT_INFO), nlohmann::json(errRes));
    }
}

// 🚀 3010: 주문 거절 핸들러
void OrderHandler::handleOrderReject(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        // 🚀 1. DTO 전체 파싱 대신, 필요한 것만 직접 추출! (가장 안전)
        auto json = nlohmann::json::parse(jsonBody);

        // "orderId"만 가져오고, 나머지는 뭐가 오든 무시합니다.
        std::string orderId = json.value("orderId", "");

        if (orderId.empty())
        {
            throw std::runtime_error("주문 번호가 누락되었습니다.");
        }

        std::cout << "[OrderHandler] 🛑 주문 거절 처리 (OrderID: " << orderId << ")" << std::endl;

        auto conn = DBManager::getInstance().getConnection();

        // 🚀 2. DB 업데이트: 상태만 9로 변경
        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
            "UPDATE ORDERS SET order_status = 9 WHERE order_id = ?"));
        pstmt->setString(1, orderId);
        pstmt->executeUpdate();

        // 🚀 3. 성공 응답 (0: 성공)
        ResOrderRejectDTO res = {0, "주문 거절 완료", orderId};
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ORDER_REJECT), nlohmann::json(res));

        // 🚀 4. 고객에게 알림
        std::string customerId = OrderDAO::getInstance().getCustomerIdByOrderId(orderId);
        if (!customerId.empty())
        {
            NotifyOrderStateDTO notify = {orderId, 9, "죄송합니다. 사장님이 주문을 거절하였습니다."};
            SessionManager::getInstance().sendToUser(customerId, static_cast<uint16_t>(CmdID::NOTIFY_ORDER_STATE), nlohmann::json(notify));
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [OrderHandler] 거절 처리 실패: " << e.what() << std::endl;
        ResOrderRejectDTO res = {1, "서버 오류", ""};
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ORDER_REJECT), nlohmann::json(res));
    }
}

// 🚀 5012: 주문 취소 핸들러
void OrderHandler::handleCancel(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        auto req = nlohmann::json::parse(jsonBody);
        std::string orderId = req.value("orderId", "");

        if (orderId.empty())
            throw std::runtime_error("주문 번호 누락");

        auto conn = DBManager::getInstance().getConnection();

        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
            "UPDATE ORDERS SET order_status = 9 WHERE order_id = ?"));
        pstmt->setString(1, orderId);
        int affected = pstmt->executeUpdate();

        if (affected > 0)
        {
            nlohmann::json res = {{"status", 200}, {"orderId", orderId}};
            session->sendPacket(static_cast<uint16_t>(CmdID::RES_CANCEL), res);

            std::string customerId = OrderDAO::getInstance().getCustomerIdByOrderId(orderId);
            if (!customerId.empty())
            {
                nlohmann::json notify = {
                    {"orderId", orderId}, {"state", 9}, {"message", "관리자에 의해 주문이 취소/환불되었습니다."}};
                SessionManager::getInstance().sendToUser(customerId, static_cast<uint16_t>(CmdID::NOTIFY_ORDER_STATE), notify);
            }

            int storeId = OrderDAO::getInstance().getStoreIdByOrderId(orderId);
            std::string ownerId = StoreDAO::getInstance().getOwnerIdByStoreId(storeId);
            if (!ownerId.empty())
            {
                nlohmann::json notify = {
                    {"orderId", orderId}, {"state", 9}, {"message", "관리자가 주문을 취소 처리했습니다."}};
                SessionManager::getInstance().sendToUser(ownerId, static_cast<uint16_t>(CmdID::NOTIFY_ORDER_STATE), notify);
            }

            std::cout << "[OrderHandler] ✅ 주문 취소 및 전방위 알림 완료 (ID: " << orderId << ")" << std::endl;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [OrderHandler] handleCancel 에러: " << e.what() << std::endl;
        nlohmann::json res = {{"status", 500}, {"message", "취소 처리 중 서버 오류"}};
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_CANCEL), res);
    }
}

// 🚀 2087: 영수증 상세 내역 핸들러
void OrderHandler::handleOrderDetail(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        auto req = nlohmann::json::parse(jsonBody);
        std::string orderId = req.value("orderId", "");

        if (orderId.empty())
            throw std::runtime_error("주문 번호가 누락되었습니다.");

        std::cout << "[OrderHandler] 🧾 영수증 상세 조회 요청 (OrderID: " << orderId << ")" << std::endl;

        auto conn = DBManager::getInstance().getConnection();
        ResOrderDetailDTO res;
        res.status = 0; // 0(성공), 1(실패)
        res.orderId = orderId;

        // 🚀 1. 3단 JOIN: 이제 O.delivery_fee와 O.wow_discount를 DB에서 직접 꺼내옵니다!
        std::unique_ptr<sql::PreparedStatement> pstmtOrder(conn->prepareStatement(
            "SELECT S.store_name, O.created_at, O.delivery_address, O.total_price, "
            "O.delivery_fee, O.wow_discount, C.card_number "
            "FROM ORDERS O "
            "JOIN STORES S ON O.store_id = S.store_id "
            "JOIN CUSTOMERS C ON O.user_id = C.user_id "
            "WHERE O.order_id = ?"));

        pstmtOrder->setString(1, orderId);
        std::unique_ptr<sql::ResultSet> rsOrder(pstmtOrder->executeQuery());

        if (rsOrder->next())
        {
            res.storeName = std::string(rsOrder->getString("store_name"));
            res.createdAt = std::string(rsOrder->getString("created_at"));
            res.deliveryAddress = std::string(rsOrder->getString("delivery_address"));
            res.totalPrice = rsOrder->getInt("total_price");
            
            // 🚀 핵심: 실시간 계산이 아닌, 과거 결제 당시의 스냅샷 데이터를 그대로 사용
            res.deliveryFee = rsOrder->getInt("delivery_fee");
            res.wowDiscount = rsOrder->getInt("wow_discount");
            res.couponDiscount = 0; // 쿠폰 할인 로직 추가 시 확장 가능

            // 결제 수단 포맷팅
            std::string cardNum = rsOrder->isNull("card_number") ? "" : std::string(rsOrder->getString("card_number"));
            if (cardNum.length() >= 4)
            {
                res.paymentMethod = "신용카드(" + cardNum.substr(cardNum.length() - 4) + ")";
            }
            else
            {
                res.paymentMethod = "신용카드 결제";
            }
        }
        else
        {
            throw std::runtime_error("해당 주문을 찾을 수 없습니다.");
        }

        // 🚀 2. 상세 메뉴 조회 (ORDER_ITEMS + MENUS JOIN)
        std::unique_ptr<sql::PreparedStatement> pstmtItems(conn->prepareStatement(
            "SELECT OI.menu_id, M.menu_name, OI.quantity, OI.unit_price, OI.selected_options "
            "FROM ORDER_ITEMS OI "
            "JOIN MENUS M ON OI.menu_id = M.menu_id "
            "WHERE OI.order_id = ?"));

        pstmtItems->setString(1, orderId);
        std::unique_ptr<sql::ResultSet> rsItems(pstmtItems->executeQuery());

        int calculatedTotalMenuPrice = 0;
        res.items.clear();

        while (rsItems->next())
        {
            OrderItemDTO item;
            item.menuId = rsItems->getInt("menu_id");
            item.menuName = std::string(rsItems->getString("menu_name"));
            item.quantity = rsItems->getInt("quantity");
            item.unitPrice = rsItems->getInt("unit_price"); 

            calculatedTotalMenuPrice += (item.unitPrice * item.quantity);

            // 옵션 JSON 파싱 방어 코드
            std::string optsStr = rsItems->isNull("selected_options") ? "[]" : std::string(rsItems->getString("selected_options"));
            try
            {
                item.selectedOptions = nlohmann::json::parse(optsStr);
            }
            catch (...)
            {
                item.selectedOptions = nlohmann::json::array();
            }
            res.items.push_back(item);
        }

        res.totalMenuPrice = calculatedTotalMenuPrice;

        std::cout << "[OrderHandler] ✅ 영수증 데이터 전송 준비 완료 (Items: " << res.items.size() << ")" << std::endl;
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ORDER_DETAIL), nlohmann::json(res));
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [OrderHandler] 상세 조회 중 오류: " << e.what() << std::endl;
        ResOrderDetailDTO errRes;
        errRes.status = 1; // 실패
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ORDER_DETAIL), nlohmann::json(errRes));
    }
}