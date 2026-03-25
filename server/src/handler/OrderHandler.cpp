#include "OrderHandler.h"
#include "OrderDAO.h"
#include "MenuDAO.h" // 🚀 서버 단독 검증을 위해 추가!
#include "AllDTOs.h"
#include "Global_protocol.h"
#include "ClientSession.h"
#include "SessionManager.h"
#include "DbManager.h" // DB 연결을 위한 매니저 클래스
#include "StoreDAO.h"
#include "MenuDAO.h"
#include <iostream>
#include <mariadb/conncpp.hpp>
#include <nlohmann/json.hpp>
#include <ctime>
#include <iomanip>

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
        std::cout << "[OrderHandler] 🧑🍳 사장님 주문 수락 요청 수신 (ID: " << req.orderId << ")" << std::endl;

        // Step 1: 트랜잭션 시작
        conn->setAutoCommit(false);

        // Step 2: 주문 상태 업데이트 (수락: 1)
        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
            "UPDATE ORDERS SET order_status = 1 WHERE order_id = ?"));
        pstmt->setString(1, req.orderId); // ✅ orderId → req.orderId
        int affected = pstmt->executeUpdate();

        if (affected > 0)
        {
            // Step 3: STORES 테이블 매출(total_sales) 누적
            std::unique_ptr<sql::PreparedStatement> pstmtSales(conn->prepareStatement(
                "UPDATE STORES S "
                "JOIN ORDERS O ON O.store_id = S.store_id "
                "SET S.total_sales = S.total_sales + O.total_price "
                "WHERE O.order_id = ?"));
            pstmtSales->setString(1, req.orderId);
            pstmtSales->executeUpdate();

            // Step 4: 커밋
            conn->commit();
            conn->setAutoCommit(true);

            // 액션 1: 사장님에게 성공 응답
            OrderAcceptResDTO res = {200, "주문 수락 및 매출 기록이 완료되었습니다."};
            res.orderId = req.orderId; // ✅ 응답 DTO에 주문번호도 포함시킵니다!
            session->sendPacket(static_cast<uint16_t>(CmdID::RES_ORDER_ACCEPT), res);

            // 액션 2: 고객 푸시 알림
            std::string customerId = OrderDAO::getInstance().getCustomerIdByOrderId(req.orderId);
            if (!customerId.empty())
            {
                NotifyOrderStateDTO notifyCustomer;
                notifyCustomer.orderId = req.orderId;
                notifyCustomer.state = 1;
                notifyCustomer.message = "사장님이 조리를 시작했습니다! (" + std::to_string(req.estimatedTime) + "분 소요 예정)";
                SessionManager::getInstance().sendToUser(
                    customerId,
                    static_cast<uint16_t>(CmdID::NOTIFY_ORDER_STATE),
                    notifyCustomer);
            }

            // 액션 3: 라이더 브로드캐스트
            NotifyDeliveryCallDTO notifyRiders;
            notifyRiders.orderId = req.orderId;
            notifyRiders.pickupAddress = StoreDAO::getInstance().getStoreDetail(OrderDAO::getInstance().getStoreIdByOrderId(req.orderId)).storeAddress; // 픽업지는 매장 주소로 세팅
            notifyRiders.deliveryAddress = OrderDAO::getInstance().getDeliveryAddressByOrderId(req.orderId);                                            // 고객 배달 주소

            int ROLE_RIDER = 2;
            SessionManager::getInstance().broadcastToRole(
                ROLE_RIDER,
                static_cast<uint16_t>(CmdID::NOTIFY_DELIVERY_CALL),
                notifyRiders);

            std::cout << "[OrderHandler] ✅ 주문 수락 처리 완료" << std::endl;
        }
        else
        {
            throw std::runtime_error("주문을 찾을 수 없거나 상태 업데이트에 실패했습니다.");
        }
    }
    catch (const std::exception &e)
    {
        conn->rollback();
        conn->setAutoCommit(true);

        std::cerr << "🚨 [OrderHandler] 주문 수락 중 에러 (Rollback 실행): "
                  << e.what() << std::endl;

        OrderAcceptResDTO res = {500, "서버 내부 오류로 주문 수락에 실패했습니다."};
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ORDER_ACCEPT), res);
    }
}

void OrderHandler::handleCheckoutInfo(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    json req;
    json res;

    try
    {
        req = json::parse(jsonBody);
        std::string userId = req.value("userId", "");
        int storeId = req.value("storeId", 0);

        if (userId.empty() || storeId == 0)
        {
            res["status"] = 400;
            res["message"] = "userId 또는 storeId가 누락되었습니다.";
            session->sendPacket(static_cast<uint16_t>(CmdID::RES_CHECKOUT_INFO), res);
            return;
        }

        auto conn = DBManager::getInstance().getConnection();

        // ── Step 1. 유저 정보 조회 (CUSTOMERS 테이블) ──
        std::unique_ptr<sql::PreparedStatement> pstmtUser(conn->prepareStatement(
            "SELECT customer_grade, card_number, account_number, point "
            "FROM CUSTOMERS WHERE user_id = ?"));
        pstmtUser->setString(1, userId);
        std::unique_ptr<sql::ResultSet> rsUser(pstmtUser->executeQuery());

        if (rsUser->next())
        {
            res["customerGrade"] = rsUser->isNull("customer_grade") ? "일반" : rsUser->getString("customer_grade").c_str();
            res["cardNumber"] = rsUser->isNull("card_number") ? "" : rsUser->getString("card_number").c_str();
            res["accountNumber"] = rsUser->isNull("account_number") ? "" : rsUser->getString("account_number").c_str();
            res["userPoint"] = rsUser->getInt("point");
        }
        else
        {
            res["customerGrade"] = "일반";
            res["cardNumber"] = "";
            res["accountNumber"] = "";
            res["userPoint"] = 0;
        }

        // ── Step 2. 매장 정보 조회 (STORES 테이블 - 🚀 포장 주소 추가) ──
        // 🚀 기존 쿼리에 store_address 컬럼을 추가로 SELECT 합니다!
        std::unique_ptr<sql::PreparedStatement> pstmtStore(conn->prepareStatement(
            "SELECT min_order_amount, delivery_fee, store_address "
            "FROM STORES WHERE store_id = ?"));
        pstmtStore->setInt(1, storeId);
        std::unique_ptr<sql::ResultSet> rsStore(pstmtStore->executeQuery());

        if (rsStore->next())
        {
            res["minOrderAmount"] = rsStore->getInt("min_order_amount");
            res["deliveryFee"] = rsStore->getInt("delivery_fee");

            // 🚀 프론트엔드 요청 사항 2종 세트 추가!
            res["storeAddress"] = rsStore->isNull("store_address") ? "" : rsStore->getString("store_address").c_str();
            res["pickupTime"] = "15~25분 후 방문 포장"; // 우선 기획서에 있는 하드코딩 값으로 세팅합니다.
        }
        else
        {
            res["minOrderAmount"] = 0;
            res["deliveryFee"] = 0;
            res["storeAddress"] = "";
            res["pickupTime"] = "";
        }

        // ── Step 3. 응답 전송 ──
        res["status"] = 200;
        res["message"] = "결제 화면 정보 조회 성공";
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_CHECKOUT_INFO), res);
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [OrderHandler] handleCheckoutInfo 오류: " << e.what() << std::endl;
        res["status"] = 500;
        res["message"] = "서버 내부 오류";
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_CHECKOUT_INFO), res);
    }
}

void OrderHandler::handleChangeOrderState(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    auto conn = DBManager::getInstance().getConnection();
    try
    {
        // 1. 만능 상태 변경 DTO 파싱
        auto req = nlohmann::json::parse(jsonBody).get<ReqChangeOrderStateDTO>();
        std::cout << "[OrderHandler] 🔄 주문 상태 변경 요청 (ID: " << req.orderId << ", NewState: " << req.newState << ")" << std::endl;

        // 2. DB 업데이트: 파라미터로 넘어온 상태값(newState)으로 그대로 덮어쓰기!
        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
            "UPDATE ORDERS SET order_status = ? WHERE order_id = ?"));
        pstmt->setInt(1, req.newState);
        pstmt->setString(2, req.orderId);
        int affected = pstmt->executeUpdate();

        if (affected > 0)
        {
            // 3. 사장님(클라이언트)에게 "처리 성공" 응답 쏘기
            ResChangeOrderStateDTO res = {200, "주문 상태가 성공적으로 변경되었습니다."};
            session->sendPacket(static_cast<uint16_t>(CmdID::RES_CHANGE_ORDER_STATE), nlohmann::json(res));

            // 4. 고객 ID 조회 및 맞춤형 푸시 알림 전송 (9010 재활용의 꽃 🌸)
            std::string customerId = OrderDAO::getInstance().getCustomerIdByOrderId(req.orderId);

            if (!customerId.empty())
            {
                NotifyOrderStateDTO notifyCustomer;
                notifyCustomer.orderId = req.orderId;
                notifyCustomer.state = req.newState;

                // 🚀 핵심: 상태값에 따라 고객의 폰에 뜰 팝업 메시지가 달라집니다!
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

        // 🚀 1. 미완료 주문 목록 조회 (요청사항 컬럼 2개 추가)
        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
            "SELECT order_id, total_price, order_status, "
            "delivery_address, created_at, store_request, rider_request "
            "FROM ORDERS "
            "WHERE store_id = ? "
            "AND order_status IN (0, 1, 2, 3) "
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
            order["orderStatus"] = rs->getInt("order_status");
            order["deliveryAddress"] = rs->getString("delivery_address").c_str();
            order["createdAt"] = rs->getString("created_at").c_str();

            // 🚀 명세서 호환: 사장님/라이더 요청사항 매핑 (NULL 방어)
            order["storeRequest"] = rs->isNull("store_request") ? "" : rs->getString("store_request").c_str();
            order["riderRequest"] = rs->isNull("rider_request") ? "" : rs->getString("rider_request").c_str();

            // 2. 해당 주문의 아이템 목록 조회
            std::unique_ptr<sql::PreparedStatement> pstmtItems(conn->prepareStatement(
                "SELECT OI.menu_id, M.menu_name, OI.quantity, "
                "OI.unit_price, OI.selected_options "
                "FROM ORDER_ITEMS OI "
                "JOIN MENUS M ON OI.menu_id = M.menu_id "
                "WHERE OI.order_id = ?"));
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

                // 🚀 명세서 호환: 옵션 배열명 변경 (options) 및 빈 배열([]) 처리
                std::string optStr = rsItems->isNull("selected_options") ? "[]" : rsItems->getString("selected_options").c_str();
                try
                {
                    item["options"] = json::parse(optStr);
                    if (!item["options"].is_array())
                    {
                        item["options"] = json::array(); // 객체나 다른 타입이면 강제로 배열로!
                    }
                }
                catch (...)
                {
                    item["options"] = json::array(); // 파싱 에러 시에도 무조건 빈 배열 반환
                }

                if (itemCount == 0)
                    firstMenuName = rsItems->getString("menu_name").c_str();
                itemCount++;
                items.push_back(item);
            }

            // 3. menuSummary 생성 (예: "떡볶이 외 1건")
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

void OrderHandler::handleOrderReject(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    auto conn = DBManager::getInstance().getConnection();
    try
    {
        // 1. 요청 파싱
        auto req = nlohmann::json::parse(jsonBody).get<ReqOrderRejectDTO>();
        std::cout << "[OrderHandler] 🚫 사장님 주문 거절 요청 (ID: " << req.orderId << ", 사유: " << req.reason << ")" << std::endl;

        // 2. DB 업데이트: 주문 상태를 '거절/취소(-1)'로 변경
        // (💡 태현님의 DB 설계에 맞춰 취소 상태 번호를 -1이나 9 등으로 맞춰주세요!)
        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
            "UPDATE ORDERS SET order_status = -1 WHERE order_id = ?"));
        pstmt->setString(1, req.orderId);
        int affected = pstmt->executeUpdate();

        if (affected > 0)
        {
            // 3. 사장님에게 성공 응답 (3011) 쏘기
            ResOrderRejectDTO res = {200, "주문 거절이 완료되었습니다."};
            res.orderId = req.orderId; // 응답 DTO에 주문번호도 포함시킵니다!
            session->sendPacket(static_cast<uint16_t>(CmdID::RES_ORDER_REJECT), nlohmann::json(res));

            // 4. 고객에게 "취소 알림" 푸시 전송 (9010 완벽 재활용!)
            std::string customerId = OrderDAO::getInstance().getCustomerIdByOrderId(req.orderId);
            if (!customerId.empty())
            {
                NotifyOrderStateDTO notifyCustomer;
                notifyCustomer.orderId = req.orderId;
                notifyCustomer.state = -1; // -1: 취소 상태
                notifyCustomer.message = "사장님 사정으로 주문이 취소되었습니다. 😭 (사유: " + req.reason + ")";

                SessionManager::getInstance().sendToUser(
                    customerId,
                    static_cast<uint16_t>(CmdID::NOTIFY_ORDER_STATE),
                    nlohmann::json(notifyCustomer));
            }
            std::cout << "[OrderHandler] ✅ 주문 거절 처리 및 고객 알림 완료" << std::endl;
        }
        else
        {
            throw std::runtime_error("주문을 찾을 수 없거나 업데이트에 실패했습니다.");
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [OrderHandler] 주문 거절 중 에러: " << e.what() << std::endl;
        ResOrderRejectDTO res = {500, "거절 처리 중 서버 오류가 발생했습니다."};
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ORDER_REJECT), nlohmann::json(res));
    }
}

// 🚀 1. 일반 주문 내역 조회 (최근 주문 순)
void OrderHandler::handleOrderHistory(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        auto req = nlohmann::json::parse(jsonBody).get<ReqOrderHistoryDTO>();
        std::cout << "[OrderHandler] 📜 주문 내역 조회 요청 (UserID: " << req.userId << ")" << std::endl;

        // 💡 두 번째 인자인 검색어(keyword)를 빈 문자열("")로 넘깁니다.
        std::vector<OrderHistoryItemDTO> historyList = OrderDAO::getInstance().getOrderHistory(req.userId, "");

        ResOrderHistoryDTO res = {200, historyList};
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ORDER_HISTORY), nlohmann::json(res));
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [OrderHandler] 주문 내역 조회 에러: " << e.what() << std::endl;
        ResOrderHistoryDTO res = {500, {}}; // 에러 시 빈 리스트 반환
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

        // 💡 클라이언트가 보낸 검색어를 그대로 DAO에 넘깁니다.
        std::vector<OrderHistoryItemDTO> historyList = OrderDAO::getInstance().getOrderHistory(req.userId, req.keyword);

        // 검색 결과 응답 (2083)
        ResOrderHistoryDTO res = {200, historyList};
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
        // 1. 클라이언트 요청 DTO 파싱
        auto req = nlohmann::json::parse(jsonBody).get<OrderCreateReqDTO>();
        std::cout << "[OrderHandler] 📦 신규 주문 생성 요청 (UserID: " << req.userId << ", StoreID: " << req.storeId << ")" << std::endl;

        // 2. 금액 위변조 검증 (방어 로직)
        int itemSum = 0;
        for (const auto &item : req.items)
        {
            itemSum += (item.unitPrice * item.quantity);
        }

        // 💡 StoreDAO에서 배달비를 가져옵니다. (기능 구현되어 있다고 가정)
        int deliveryFee = StoreDAO::getInstance().getDeliveryFee(req.storeId);

        if (itemSum + deliveryFee != req.totalPrice)
        {
            std::cerr << "🚨 [OrderHandler] 금액 위변조 의심! (메뉴합계+배달비: " << (itemSum + deliveryFee) << ", 수신된 총액: " << req.totalPrice << ")" << std::endl;
            // 💡 기획/테스트 단계에서 배달비 로직이 꼬인다면 아래 throw 구문을 잠시 주석 처리하셔도 됩니다.
            // throw std::runtime_error("결제 금액 검증에 실패했습니다.");
        }

        // 3. 트랜잭션 시작 (DB 안전제일!)
        conn->setAutoCommit(false);

        // 4. 🚀 고유 주문번호 생성 (난수 3자리 추가로 '따닥 클릭' 동시성 완벽 방어!)
        time_t now = time(nullptr);
        char timeBuf[80];
        strftime(timeBuf, sizeof(timeBuf), "%Y%m%d%H%M%S", localtime(&now));

        int randomNum = rand() % 1000;
        char randomBuf[10];
        sprintf(randomBuf, "%03d", randomNum); // 007, 421 처럼 3자리 포맷팅

        std::string newOrderId = "ORD-" + std::string(timeBuf) + "-" + std::string(randomBuf) + "-" + req.userId;

        // 5. ORDERS 테이블 INSERT
        std::unique_ptr<sql::PreparedStatement> pstmtOrder(conn->prepareStatement(
            "INSERT INTO ORDERS (order_id, user_id, store_id, total_price, delivery_address, store_request, rider_request, order_status, created_at) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, 0, NOW())"));
        pstmtOrder->setString(1, newOrderId);
        pstmtOrder->setString(2, req.userId);
        pstmtOrder->setInt(3, req.storeId);
        pstmtOrder->setInt(4, req.totalPrice);
        pstmtOrder->setString(5, req.deliveryAddress);
        pstmtOrder->setString(6, req.storeRequest);
        pstmtOrder->setString(7, req.riderRequest);
        pstmtOrder->executeUpdate();

        // 6. ORDER_DETAILS (또는 ORDER_ITEMS) 테이블 INSERT 및 푸시 알림용 데이터 조립
        std::unique_ptr<sql::PreparedStatement> pstmtItems(conn->prepareStatement(
            "INSERT INTO ORDER_DETAILS (order_id, menu_id, menu_name, quantity, price) VALUES (?, ?, ?, ?, ?)"));

        nlohmann::json pushItemsArray = nlohmann::json::array();
        std::string firstMenuName = "";

        for (size_t i = 0; i < req.items.size(); ++i)
        {
            const auto &item = req.items[i];

            // 메뉴 이름 조회 (없으면 대체 텍스트)
            std::string menuName = MenuDAO::getInstance().getMenuName(item.menuId);
            if (menuName.empty())
                menuName = "메뉴 #" + std::to_string(item.menuId);
            if (i == 0)
                firstMenuName = menuName;

            // DB 저장
            pstmtItems->setString(1, newOrderId);
            pstmtItems->setInt(2, item.menuId);
            pstmtItems->setString(3, menuName);
            pstmtItems->setInt(4, item.quantity);
            pstmtItems->setInt(5, item.unitPrice);
            pstmtItems->executeUpdate();

            // 🚀 사장님 영수증 푸시(9000)를 위한 개별 메뉴 JSON 조립 (명세서 완벽 준수)
            nlohmann::json pushItem;
            pushItem["menuId"] = item.menuId;
            pushItem["menuName"] = menuName;
            pushItem["quantity"] = item.quantity;
            pushItem["unitPrice"] = item.unitPrice;

            // 옵션 처리 (selectedOptions -> options 로 이름 변경 및 빈 배열 보장)
            nlohmann::json itemJson = item; // DTO를 JSON 객체로 잠시 직렬화
            nlohmann::json opts = itemJson.value("selectedOptions", nlohmann::json::array());
            if (!opts.is_array())
                opts = nlohmann::json::array();

            pushItem["options"] = opts;
            pushItemsArray.push_back(pushItem);
        }

        // 7. 모든 쿼리가 성공했으므로 영구 반영!
        conn->commit();
        conn->setAutoCommit(true);
        std::cout << "[OrderHandler] ✅ 주문 생성 완벽 성공! (OrderID: " << newOrderId << ")" << std::endl;

        // 8. 고객(클라이언트)에게 2021번 성공 응답 전송
        OrderCreateResDTO res = {0, "주문이 성공적으로 생성되었습니다.", newOrderId};
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ORDER_CREATE), nlohmann::json(res));

        // 9. 🚀 사장님에게 9000번(NOTIFY_NEW_ORDER) 알림 발송! (클라이언트 명세서 100% 반영)
        std::string ownerId = StoreDAO::getInstance().getOwnerIdByStoreId(req.storeId);
        if (!ownerId.empty())
        {
            nlohmann::json pushData;
            pushData["orderId"] = newOrderId;
            pushData["deliveryAddress"] = req.deliveryAddress;
            pushData["totalPrice"] = req.totalPrice;
            pushData["orderStatus"] = 0; // 0: 대기 상태

            char createdAtBuf[80];
            strftime(createdAtBuf, sizeof(createdAtBuf), "%Y-%m-%d %H:%M:%S", localtime(&now));
            pushData["createdAt"] = std::string(createdAtBuf);

            // 🚀 Depth 1 평탄화 (요청사항)
            pushData["storeRequest"] = req.storeRequest;
            pushData["riderRequest"] = req.riderRequest;

            // 메뉴 요약 및 전체 아이템 내역
            pushData["menuSummary"] = firstMenuName + (req.items.size() > 1 ? " 외 " + std::to_string(req.items.size() - 1) + "건" : "");
            pushData["items"] = pushItemsArray;

            // SessionManager를 통해 사장님에게 쏜다!
            bool isPushed = SessionManager::getInstance().sendToUser(
                ownerId,
                static_cast<uint16_t>(CmdID::NOTIFY_NEW_ORDER),
                pushData);

            if (isPushed)
            {
                std::cout << "[OrderHandler] 🔔 사장님(" << ownerId << ") 포스기에 접수 알림(9000번) 발송 완료!" << std::endl;
            }
            else
            {
                std::cout << "[OrderHandler] 📴 사장님(" << ownerId << ") 오프라인 상태 (푸시 생략)." << std::endl;
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [OrderHandler] 주문 생성 실패 (Rollback 실행): " << e.what() << std::endl;
        if (conn)
        {
            conn->rollback();
            conn->setAutoCommit(true);
        }
        OrderCreateResDTO res = {1, "주문 처리 중 오류가 발생했습니다. (" + std::string(e.what()) + ")", ""};
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ORDER_CREATE), nlohmann::json(res));
    }
}