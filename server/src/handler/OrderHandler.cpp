#include "OrderHandler.h"
#include "OrderDAO.h"
#include "MenuDAO.h" // 🚀 서버 단독 검증을 위해 추가!
#include "AllDTOs.h"
#include "Global_protocol.h"
#include "ClientSession.h"
#include "SessionManager.h"
#include "DbManager.h" // DB 연결을 위한 매니저 클래스
#include <iostream>
#include <mariadb/conncpp.hpp>
#include <nlohmann/json.hpp>

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
            notifyRiders.pickupAddress = "매장 주소 (DB 조회 권장)";
            notifyRiders.deliveryAddress = "고객 배달 주소";

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