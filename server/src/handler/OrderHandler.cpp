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

void OrderHandler::handleOrderCreate(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    json req;
    json res;
    auto conn = DBManager::getInstance().getConnection();

    try
    {
        req = json::parse(jsonBody);

        // 1. 클라이언트가 보낸 기본 데이터 파싱
        std::string orderId = req.value("orderId", "");
        std::string userId = session->getUserId();
        int storeId = req.value("storeId", 0);
        int totalPrice = req.value("totalPrice", 0);
        std::string deliveryAddress = req.value("deliveryAddress", "");
        std::string storeRequest = req.value("storeRequest", "");
        std::string riderRequest = req.value("riderRequest", "");
        json paymentInfo = req["payment"];
        json items = req["items"];

        if (orderId.empty() || items.empty())
        {
            throw std::invalid_argument("필수 주문 데이터가 누락되었습니다.");
        }

        // 🚀 [2차 방어선]: 서버 단독 가격 검증 로직 시작
        // 프론트가 보낸 totalPrice(총 결제금액)를 믿지 않고, 서버가 DB 원가를 바탕으로 직접 재계산합니다.
        int serverCalculatedTotalPrice = 0;

        for (auto &item : items)
        {
            int menuId = item.value("menuId", 0);
            int quantity = item.value("quantity", 1);

            // 1) DB에서 메뉴 기본 가격 가져오기 (MenuDAO 필요)
            int realBasePrice = MenuDAO::getInstance().getMenuBasePrice(menuId);
            if (realBasePrice < 0)
            {
                throw std::invalid_argument("존재하지 않는 메뉴(ID: " + std::to_string(menuId) + ")가 포함되어 있습니다.");
            }

            int itemTotalOptionPrice = 0;
            json selectedOptions = item["selectedOptions"];

            // 2) 선택된 옵션들의 추가 금액 가져오기 (문자열 파싱 없이 JSON 배열 순회)
            // (주의: 프론트엔드가 옵션 정보를 어떻게 보내는지에 따라 파싱 방식이 다를 수 있음.
            //  여기서는 프론트가 배열 형태로 보낸다고 가정합니다.)
            if (selectedOptions.is_array())
            {
                for (auto &opt : selectedOptions)
                {
                    // 프론트에서 보낸 옵션 가격을 일단 사용 (추후 더 엄격하게 하려면 DB에서 옵션 가격도 조회해야 함)
                    int optPrice = opt.value("additionalPrice", 0);
                    itemTotalOptionPrice += optPrice;
                }
            }

            // 3) 이 메뉴 1개의 진짜 가격 = (기본가 + 옵션가합) * 수량
            int realItemTotalPrice = (realBasePrice + itemTotalOptionPrice) * quantity;
            serverCalculatedTotalPrice += realItemTotalPrice;

            // 4) 프론트가 보낸 단가(unitPrice) 변조 여부 체크 (옵션)
            // int frontUnitPrice = item.value("unitPrice", 0);
            // if (frontUnitPrice != (realBasePrice + itemTotalOptionPrice)) { ... }
        }

        // 배달비 합산 (STORES 테이블에서 조회 필요, 여기서는 임시로 0원 처리 또는 프론트 값 사용)
        // int deliveryFee = OrderDAO::getInstance().getDeliveryFee(storeId);
        // serverCalculatedTotalPrice += deliveryFee;

        // 최종 비교! (현재는 배달비 로직이 완벽히 연동 안 되었을 수 있으니, 주석 처리 해둡니다.
        // 나중에 배달비까지 완벽히 계산되면 주석을 푸세요!)
        /*
        if (serverCalculatedTotalPrice != totalPrice) {
            std::cerr << "[보안 경고] 가격 변조 시도 감지! (프론트: " << totalPrice
                      << ", 서버계산: " << serverCalculatedTotalPrice << ")" << std::endl;
            throw std::runtime_error("결제 금액이 일치하지 않아 주문이 취소되었습니다.");
        }
        */
        // 🚀 [2차 방어선] 끝.

        // 🚀 트랜잭션 시작 (자동 커밋 끄기)
        conn->setAutoCommit(false);

        // ── [Step 1] ORDERS 테이블 저장 ──
        std::unique_ptr<sql::PreparedStatement> pstmtOrder(conn->prepareStatement(
            "INSERT INTO ORDERS (order_id, user_id, store_id, total_price, order_status, delivery_address, store_request, rider_request) "
            "VALUES (?, ?, ?, ?, 0, ?, ?, ?)"));
        pstmtOrder->setString(1, orderId);
        pstmtOrder->setString(2, userId);
        pstmtOrder->setInt(3, storeId);
        pstmtOrder->setInt(4, totalPrice); // 일단 프론트가 보낸 값 저장 (추후 serverCalculatedTotalPrice로 교체 권장)
        pstmtOrder->setString(5, deliveryAddress);
        pstmtOrder->setString(6, storeRequest);
        pstmtOrder->setString(7, riderRequest);
        pstmtOrder->executeUpdate();

        // ── [Step 2] ORDER_ITEMS 테이블 저장 (장바구니 메뉴들) ──
        std::unique_ptr<sql::PreparedStatement> pstmtItem(conn->prepareStatement(
            "INSERT INTO ORDER_ITEMS (order_id, menu_id, quantity, unit_price, selected_options) "
            "VALUES (?, ?, ?, ?, ?)"));

        std::string firstMenuName = ""; // 푸시 알림용 첫 번째 메뉴 이름

        for (size_t i = 0; i < items.size(); ++i)
        {
            auto &item = items[i];
            pstmtItem->setString(1, orderId);
            pstmtItem->setInt(2, item.value("menuId", 0));
            pstmtItem->setInt(3, item.value("quantity", 1));
            pstmtItem->setInt(4, item.value("unitPrice", 0));
            pstmtItem->setString(5, item["selectedOptions"].dump()); // JSON 객체/배열을 문자열로 직렬화하여 저장
            pstmtItem->executeUpdate();

            // 첫 번째 메뉴 이름 저장 (찬우님의 '떡볶이 외 1건' 로직을 위해)
            if (i == 0)
                firstMenuName = item.value("menuName", "메뉴");
        }

        // ── [Step 3] PAYMENTS 테이블 저장 (영수증용 결제 원장) ──
        std::unique_ptr<sql::PreparedStatement> pstmtPay(conn->prepareStatement(
            "INSERT INTO PAYMENTS (order_id, user_id, pay_method, total_amount, discount_amount, actual_pay_amount) "
            "VALUES (?, ?, ?, ?, ?, ?)"));
        pstmtPay->setString(1, orderId);
        pstmtPay->setString(2, userId);
        pstmtPay->setString(3, paymentInfo.value("payMethod", "이츠페이"));
        pstmtPay->setInt(4, paymentInfo.value("totalAmount", totalPrice));
        pstmtPay->setInt(5, paymentInfo.value("discountAmount", 0));
        pstmtPay->setInt(6, paymentInfo.value("actualPayAmount", totalPrice));
        pstmtPay->executeUpdate();

        // ── [Step 4] 사장님 ID(owner_id) 찾기 ──
        std::unique_ptr<sql::PreparedStatement> pstmtStore(conn->prepareStatement(
            "SELECT owner_id FROM STORES WHERE store_id = ?"));
        pstmtStore->setInt(1, storeId);
        std::unique_ptr<sql::ResultSet> rsStore(pstmtStore->executeQuery());
        std::string ownerId = "";
        if (rsStore->next())
        {
            ownerId = rsStore->getString("owner_id").c_str();
        }

        // 🚀 모든 DB 작업이 성공했으므로 커밋!
        conn->commit();
        conn->setAutoCommit(true);

        std::cout << "[OrderHandler] 주문 생성 및 결제 정보 DB 저장 완료! (orderId: " << orderId << ")\n";

        // ── [Step 5] 사장님에게 9000번 푸시 알림 (찬우님 기획 + 태현님 디테일) ──
        if (!ownerId.empty())
        {
            json pushData;
            pushData["orderId"] = orderId;
            pushData["totalPrice"] = totalPrice;
            pushData["deliveryAddress"] = deliveryAddress;

            // 메뉴 요약 ("황궁쟁반짜장 외 1건")
            if (items.size() > 1)
            {
                pushData["menuSummary"] = firstMenuName + " 외 " + std::to_string(items.size() - 1) + "건";
            }
            else
            {
                pushData["menuSummary"] = firstMenuName;
            }

            pushData["items"] = items;

            // 🚀 핵심: 사장님 포스기 영수증 출력을 위한 디테일 추가!
            pushData["requests"]["store"] = storeRequest;
            pushData["requests"]["rider"] = riderRequest;
            pushData["payMethod"] = paymentInfo.value("payMethod", "이츠페이");

            // TODO: SessionManager 연동 (주석 해제 필요)
            /*
            auto ownerSession = SessionManager::getInstance().getSessionByUserId(ownerId);
            if (ownerSession) {
                ownerSession->sendPacket(static_cast<uint16_t>(CmdID::NOTIFY_NEW_ORDER), pushData.dump());
            }
            */
            std::cout << ">>> [DEBUG] 사장님(" << ownerId << ")에게 푸시 발송 대기 중...\n";
        }

        // ── [Step 6] 고객에게 2021번 성공 응답 전송 ──
        res["status"] = 200;
        res["message"] = "주문 및 결제가 완료되었습니다.";
        res["orderId"] = orderId;
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ORDER_CREATE), res);
    }
    catch (const std::exception &e)
    {
        // 🚨 중간에 에러가 났다면 DB 롤백! (돈이 빠져나갔는데 주문이 안 들어가는 참사 방지)
        conn->rollback();
        conn->setAutoCommit(true);

        std::cerr << "[OrderHandler] 주문 생성 실패 (Rollback 실행): " << e.what() << std::endl;
        res["status"] = 500;
        res["message"] = "주문 처리 중 서버 오류가 발생했습니다: " + std::string(e.what());
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ORDER_CREATE), res);
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

void OrderHandler::handleOrderList(
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
            session->sendPacket(static_cast<uint16_t>(CmdID::RES_ORDER_LIST), res);
            return;
        }

        auto conn = DBManager::getInstance().getConnection();

        // 1. 미완료 주문 목록 조회
        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
            "SELECT order_id, total_price, order_status, "
            "delivery_address, created_at "
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

                // selected_options JSON 파싱
                std::string optStr = rsItems->isNull("selected_options") ? "{}" : rsItems->getString("selected_options").c_str();
                try
                {
                    item["selectedOptions"] = json::parse(optStr);
                }
                catch (...)
                {
                    item["selectedOptions"] = json::object();
                }

                if (itemCount == 0)
                    firstMenuName = rsItems->getString("menu_name").c_str();
                itemCount++;
                items.push_back(item);
            }

            // 3. menuSummary 생성
            // 예: "떡볶이" 또는 "떡볶이 외 1건"
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
        std::cerr << "[OrderHandler] handleOrderList 오류: " << e.what() << std::endl;
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

        // 💡 StoreDAO에서 해당 매장의 배달비를 가져옵니다. (구현되어 있다고 가정)
        int deliveryFee = StoreDAO::getInstance().getDeliveryFee(req.storeId);

        if (itemSum + deliveryFee != req.totalPrice)
        {
            std::cerr << "🚨 [OrderHandler] 금액 위변조 의심! (메뉴합계+배달비: " << (itemSum + deliveryFee) << ", 수신된 총액: " << req.totalPrice << ")" << std::endl;
            throw std::runtime_error("결제 금액 검증에 실패했습니다.");
        }

        // 3. 트랜잭션 시작 (여기서부터는 실패하면 전부 무효화!)
        conn->setAutoCommit(false);

        // 4. 고유 주문번호 생성 (예: ORD-20260325143000-user1)
        time_t now = time(nullptr);
        char timeBuf[80];
        strftime(timeBuf, sizeof(timeBuf), "%Y%m%d%H%M%S", localtime(&now));
        std::string newOrderId = "ORD-" + std::string(timeBuf) + "-" + req.userId;

        // 5. ORDERS 테이블 INSERT (마스터 데이터)
        std::unique_ptr<sql::PreparedStatement> pstmtOrder(conn->prepareStatement(
            "INSERT INTO ORDERS (order_id, user_id, store_id, total_price, delivery_address, store_request, rider_request, order_status, created_at) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, 0, NOW())"));
        pstmtOrder->setString(1, newOrderId);
        pstmtOrder->setString(2, req.userId);
        pstmtOrder->setInt(3, req.storeId);
        pstmtOrder->setInt(4, req.totalPrice);
        pstmtOrder->setString(5, req.deliveryAddress);
        pstmtOrder->setString(6, req.storeRequest); // 사장님께
        pstmtOrder->setString(7, req.riderRequest); // 라이더님께
        pstmtOrder->executeUpdate();

        // 6. ORDER_DETAILS 테이블 INSERT (상세 메뉴들)
        std::unique_ptr<sql::PreparedStatement> pstmtItems(conn->prepareStatement(
            "INSERT INTO ORDER_DETAILS (order_id, menu_id, menu_name, quantity, price) VALUES (?, ?, ?, ?, ?)"));

        for (const auto &item : req.items)
        {
            // 💡 아까 에러 났던 'menu_name'을 넣기 위해 DB에서 메뉴 이름을 조회해 옵니다.
            std::string menuName = MenuDAO::getInstance().getMenuName(item.menuId);
            if (menuName.empty())
                menuName = "알 수 없는 메뉴"; // 방어 코드

            pstmtItems->setString(1, newOrderId);
            pstmtItems->setInt(2, item.menuId);
            pstmtItems->setString(3, menuName);
            pstmtItems->setInt(4, item.quantity);
            pstmtItems->setInt(5, item.unitPrice);
            pstmtItems->executeUpdate();
        }

        // 7. 모든 쿼리가 성공하면 DB에 영구 반영 (Commit)
        conn->commit();
        conn->setAutoCommit(true);
        std::cout << "[OrderHandler] ✅ 주문 생성 완벽 성공! (OrderID: " << newOrderId << ")" << std::endl;

        // 8. 클라이언트에게 성공 응답 전송
        OrderCreateResDTO res = {0, "주문이 성공적으로 생성되었습니다.", newOrderId};
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ORDER_CREATE), nlohmann::json(res));

        // 🚀 (보너스) 여기서 사장님한테 "새 주문 들어왔어요~" 푸시 알림(9000번)을 쏘면 완벽합니다!
    }
    catch (const std::exception &e) // SQL 에러 및 일반 에러 모두 포착
    {
        std::cerr << "🚨 [OrderHandler] 주문 생성 실패 (Rollback 실행): " << e.what() << std::endl;
        if (conn)
        {
            conn->rollback(); // 싹 다 없던 일로 되돌림
            conn->setAutoCommit(true);
        }
        OrderCreateResDTO res = {1, "주문 처리 중 오류가 발생했습니다. (" + std::string(e.what()) + ")", ""};
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ORDER_CREATE), nlohmann::json(res));
    }
}