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
// 🛒 1. 고객의 주문 생성 요청 (2020번) - 2차 방어선 탑재!
// =========================================================================
void OrderHandler::handleOrderCreate(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        auto req = nlohmann::json::parse(jsonBody).get<OrderCreateReqDTO>();
        std::cout << "[OrderHandler] 🛒 유저 '" << req.userId << "'의 장바구니 결제 요청 수신" << std::endl;

        // 🛡️ =================================================================
        // [2차 방어선] 데이터 무결성 검증 (클라이언트 데이터를 절대 믿지 않음)
        // ===================================================================
        int calculatedTotal = 0; // 서버가 직접 계산할 '진짜 총액'

        for (const auto &item : req.items)
        {
            // 🚨 방어 1: 상점 크로스체크 (A식당 주문에 B식당 메뉴가 섞여있는지 확인)
            auto dbMenuInfo = MenuDAO::getInstance().getMenuBasicInfo(item.menuId);

            // 만약 없는 메뉴이거나, 요청한 상점(req.storeId)과 메뉴의 실제 상점이 다르면 차단
            if (dbMenuInfo.storeId == 0 || dbMenuInfo.storeId != req.storeId)
            {
                std::cerr << "🚨 [OrderHandler] 상점 불일치 감지! (메뉴 해킹 의심)" << std::endl;
                OrderCreateResDTO res = {1, "유효하지 않은 메뉴가 포함되어 있습니다.", ""};
                session->sendPacket(static_cast<uint16_t>(CmdID::RES_ORDER_CREATE), res);
                return; // 함수 즉시 종료 (DAO로 안 넘어감)
            }

            // 🚨 방어 2: 옵션 가격 직접 합산 (selectedOptions가 JSON 배열이라고 가정)
            int optionTotal = 0;
            if (item.selectedOptions.is_array())
            {
                for (const auto &opt : item.selectedOptions)
                {
                    // 클라이언트가 보낸 옵션 ID 추출 (프론트 JSON 구조에 따라 키 이름은 맞추세요)
                    int optId = opt.value("optionId", 0);
                    if (optId > 0)
                    {
                        optionTotal += MenuDAO::getInstance().getOptionPrice(optId);
                    }
                }
            }

            // 🚨 방어 3: 아이템별 진짜 단가 계산 (서버 DB 기준 원가 + 옵션가)
            int realUnitPrice = dbMenuInfo.basePrice + optionTotal;

            // 서버가 계산한 진짜 단가 * 수량을 총액에 누적
            calculatedTotal += (realUnitPrice * item.quantity);
        }

        // 🚨 방어 4: 최종 결제 금액 대조 (프론트 총액 vs 서버 총액)
        // (참고: 만약 배달비가 있다면 calculatedTotal += 배달비; 처리가 필요합니다!)
        if (calculatedTotal != req.totalPrice)
        {
            std::cerr << "🚨 [OrderHandler] 금액 위변조 감지! (요청액: " << req.totalPrice << ", 서버계산액: " << calculatedTotal << ")" << std::endl;
            OrderCreateResDTO res = {1, "결제 금액이 일치하지 않습니다. 장바구니를 다시 확인해주세요.", ""};
            session->sendPacket(static_cast<uint16_t>(CmdID::RES_ORDER_CREATE), res);
            return; // 함수 즉시 종료
        }
        // ===================================================================

        // 🛡️ 방어선을 모두 통과한 '깨끗한 데이터'만 DB 저장(DAO)으로 넘깁니다.
        OrderResult dbResult = OrderDAO::getInstance().createOrder(req);

        OrderCreateResDTO res;
        res.status = dbResult.isSuccess ? 0 : 1;
        res.message = dbResult.message;
        res.orderId = dbResult.orderId;

        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ORDER_CREATE), res);
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [OrderHandler] 주문 생성 중 에러: " << e.what() << std::endl;
        OrderCreateResDTO res = {1, "서버 내부 오류로 주문에 실패했습니다.", ""};
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ORDER_CREATE), res);
    }
}

// =========================================================================
// 🧑‍🍳 2. 사장님의 주문 수락 요청 (3000번)
// =========================================================================
void OrderHandler::handleOrderAccept(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        auto req = nlohmann::json::parse(jsonBody).get<OrderAcceptReqDTO>();
        std::cout << "[OrderHandler] 🧑‍🍳 사장님이 주문(" << req.orderId << ")을 수락. 예상시간: " << req.estimatedTime << "분" << std::endl;

        bool isDbSuccess = OrderDAO::getInstance().updateOrderStatus(req.orderId, 1);

        if (isDbSuccess)
        {
            // 액션 1: 사장님 응답
            OrderAcceptResDTO res = {0, "주문이 성공적으로 수락되었습니다."};
            session->sendPacket(static_cast<uint16_t>(CmdID::RES_ORDER_ACCEPT), res);

            // 액션 2: 고객 푸시 알림
            NotifyOrderStateDTO notifyCustomer;
            notifyCustomer.orderId = req.orderId;
            notifyCustomer.state = 1;
            notifyCustomer.message = "사장님이 조리를 시작했습니다! (" + std::to_string(req.estimatedTime) + "분 소요 예정)";

            std::string customerId = OrderDAO::getInstance().getCustomerIdByOrderId(req.orderId);
            SessionManager::getInstance().sendToUser(customerId, static_cast<uint16_t>(CmdID::NOTIFY_ORDER_STATE), notifyCustomer);

            // 액션 3: 라이더 브로드캐스트
            NotifyDeliveryCallDTO notifyRiders;
            notifyRiders.orderId = req.orderId;
            notifyRiders.pickupAddress = "황궁짜장 (송정점)";
            notifyRiders.deliveryAddress = "고객 배달 주소";

            int ROLE_RIDER = 2;
            SessionManager::getInstance().broadcastToRole(ROLE_RIDER, static_cast<uint16_t>(CmdID::NOTIFY_DELIVERY_CALL), notifyRiders);
        }
        else
        {
            OrderAcceptResDTO res = {1, "DB 오류로 주문 수락에 실패했습니다."};
            session->sendPacket(static_cast<uint16_t>(CmdID::RES_ORDER_ACCEPT), res);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [OrderHandler] 주문 수락 중 에러: " << e.what() << std::endl;
    }
}

void OrderHandler::handleCheckoutInfo(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    json req = json::parse(jsonBody);
    json res;

    try
    {
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

        // ── 1. 유저 정보 조회 (CUSTOMERS 테이블) ──
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
            // 유저 정보가 아직 CUSTOMERS 테이블에 없다면 기본값 세팅
            res["customerGrade"] = "일반";
            res["cardNumber"] = "";
            res["accountNumber"] = "";
            res["userPoint"] = 0;
        }

        // ── 2. 매장 정보 조회 (STORES 테이블) ──
        std::unique_ptr<sql::PreparedStatement> pstmtStore(conn->prepareStatement(
            "SELECT min_order_amount, delivery_fee "
            "FROM STORES WHERE store_id = ?"));
        pstmtStore->setInt(1, storeId);
        std::unique_ptr<sql::ResultSet> rsStore(pstmtStore->executeQuery());

        if (rsStore->next())
        {
            res["minOrderAmount"] = rsStore->getInt("min_order_amount");
            res["deliveryFee"] = rsStore->getInt("delivery_fee");
        }
        else
        {
            res["minOrderAmount"] = 0;
            res["deliveryFee"] = 0;
        }

        // ── 3. 최종 응답 전송 ──
        res["status"] = 200;
        res["message"] = "결제 화면 정보 조회 성공";
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_CHECKOUT_INFO), res);
    }
    catch (const std::exception &e)
    {
        std::cerr << "[OrderHandler] handleCheckoutInfo 오류: " << e.what() << std::endl;
        res["status"] = 500;
        res["message"] = "서버 내부 오류";
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_CHECKOUT_INFO), res);
    }
}