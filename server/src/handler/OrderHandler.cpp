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

        std::cout << "[OrderHandler] 🧑🍳 사장님 주문 수락 요청 수신 (ID: " << req.orderId << ")" << std::endl;

        conn->setAutoCommit(false);

        // 🚀 핵심: 서버 로그에 찍힌 그 주문번호("ORD-...-886-jina")가 DB에서 0인지 확인하며 업데이트!
        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
            "UPDATE ORDERS SET order_status = 1 WHERE order_id = ? AND order_status = 0"));
        pstmt->setString(1, req.orderId);
        int affected = pstmt->executeUpdate();

        if (affected == 0)
        {
            // 💡 팁: 여기서 에러가 난다면 DB에서 해당 ID의 order_status가 진짜 0인지 꼭 확인하세요!
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

        // 1. 미완료 주문 목록 조회
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

        // 🚀 루프 시작: 여기서 orderId가 만들어집니다!
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

            // 🚀 2. ORDER_DETAILS 테이블에서 메뉴 리스트 조회 (JOIN 불필요!)
            std::unique_ptr<sql::PreparedStatement> pstmtItems(conn->prepareStatement(
                "SELECT menu_id, menu_name, quantity, "
                "price AS unit_price, selected_options "
                "FROM ORDER_DETAILS "
                "WHERE order_id = ?"));
            pstmtItems->setString(1, orderId); // 👈 위에서 만든 orderId를 여기서 사용!
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

        int deliveryFee = StoreDAO::getInstance().getDeliveryFee(req.storeId);
        if (itemSum + deliveryFee != req.totalPrice)
        {
            std::cerr << "🚨 [OrderHandler] 금액 변조 의심! (계산됨: " << (itemSum + deliveryFee) << ", 수신됨: " << req.totalPrice << ")" << std::endl;
        }

        // 3. 트랜잭션 시작
        conn->setAutoCommit(false);

        // 4. 🚀 고유 주문번호 생성 (난수 추가로 동시성 방어)
        time_t now = time(nullptr);
        char timeBuf[80];
        strftime(timeBuf, sizeof(timeBuf), "%Y%m%d%H%M%S", localtime(&now));
        int randomNum = rand() % 1000;
        char randomBuf[10];
        sprintf(randomBuf, "%03d", randomNum);

        std::string newOrderId = "ORD-" + std::string(timeBuf) + "-" + std::string(randomBuf) + "-" + req.userId;

        // 🚀 5. ORDERS 테이블 INSERT (심장 복구!)
        // DB에 주문의 '부모' 데이터를 먼저 생성해야 합니다.
        std::unique_ptr<sql::PreparedStatement> pstmtOrder(conn->prepareStatement(
            "INSERT INTO ORDERS (order_id, user_id, store_id, total_price, delivery_address, store_request, rider_request, order_status, created_at) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, 0, NOW())")); // 💡 상태값 0(대기중)으로 명시!

        pstmtOrder->setString(1, newOrderId);
        pstmtOrder->setString(2, req.userId);
        pstmtOrder->setInt(3, req.storeId);
        pstmtOrder->setInt(4, req.totalPrice);
        pstmtOrder->setString(5, req.deliveryAddress);
        pstmtOrder->setString(6, req.storeRequest);
        pstmtOrder->setString(7, req.riderRequest);
        pstmtOrder->executeUpdate(); // 👈 여기서 드디어 DB에 주문이 생성됩니다!

        // 🚀 6. ORDER_DETAILS 테이블 INSERT 및 메뉴 리스트 조립
        std::unique_ptr<sql::PreparedStatement> pstmtItems(conn->prepareStatement(
            "INSERT INTO ORDER_DETAILS (order_id, menu_id, menu_name, quantity, price, selected_options) VALUES (?, ?, ?, ?, ?, ?)"));

        nlohmann::json pushItemsArray = nlohmann::json::array();
        std::string firstMenuName = "";

        for (size_t i = 0; i < req.items.size(); ++i)
        {
            const auto &item = req.items[i];

            // DB에서 진짜 메뉴 이름을 찾습니다.
            std::string menuName = MenuDAO::getInstance().getMenuName(item.menuId);
            if (menuName.empty())
                menuName = "메뉴 #" + std::to_string(item.menuId);
            if (i == 0)
                firstMenuName = menuName;

            // 🚀 한글 옵션 이름 추출 로직 (태현님 설계 반영)
            nlohmann::json opts = item.selectedOptions; // DTO에서 가져온 json 객체
            nlohmann::json optionNamesArray = nlohmann::json::array();

            if (opts.is_array())
            {
                for (const auto &optId : opts)
                {
                    if (optId.is_number_integer())
                    {
                        // 💡 menuId와 optId를 함께 넘겨 JSON에서 한글 이름을 찾아옵니다!
                        std::string optName = MenuDAO::getInstance().getOptionName(item.menuId, optId.get<int>());
                        if (!optName.empty())
                            optionNamesArray.push_back(optName);
                    }
                }
            }

            // DB 저장 (selected_options 컬럼에 한글 이름 배열을 문자열로 저장)
            pstmtItems->setString(1, newOrderId);
            pstmtItems->setInt(2, item.menuId);
            pstmtItems->setString(3, menuName);
            pstmtItems->setInt(4, item.quantity);
            pstmtItems->setInt(5, item.unitPrice);
            pstmtItems->setString(6, optionNamesArray.dump());
            pstmtItems->executeUpdate();

            // 사장님 푸시를 위한 메뉴 객체 생성
            nlohmann::json pushItem;
            pushItem["menuId"] = item.menuId;
            pushItem["menuName"] = menuName;
            pushItem["quantity"] = item.quantity;
            pushItem["unitPrice"] = item.unitPrice;
            pushItem["options"] = optionNamesArray; // 한글 이름 배열 탑재!

            pushItemsArray.push_back(pushItem);
        }

        // 7. DB 영구 반영 (Commit)
        conn->commit();
        conn->setAutoCommit(true);
        std::cout << "[OrderHandler] ✅ 주문 생성 및 DB 저장 완벽 성공! (OrderID: " << newOrderId << ")" << std::endl;

        // 8. 고객에게 성공 응답
        OrderCreateResDTO res = {0, "주문이 성공적으로 생성되었습니다.", newOrderId};
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ORDER_CREATE), nlohmann::json(res));

        // 9. 사장님에게 9000번 알림 발송!
        std::string ownerId = StoreDAO::getInstance().getOwnerIdByStoreId(req.storeId);
        if (!ownerId.empty())
        {
            nlohmann::json pushData;
            pushData["orderId"] = newOrderId;
            pushData["deliveryAddress"] = req.deliveryAddress;
            pushData["totalPrice"] = req.totalPrice;
            pushData["state"] = 0; // 💡 클라이언트 규격 'state'로 통일!

            char createdAtBuf[80];
            strftime(createdAtBuf, sizeof(createdAtBuf), "%Y-%m-%d %H:%M:%S", localtime(&now));
            pushData["createdAt"] = std::string(createdAtBuf);
            pushData["storeRequest"] = req.storeRequest;
            pushData["riderRequest"] = req.riderRequest;
            pushData["menuSummary"] = firstMenuName + (req.items.size() > 1 ? " 외 " + std::to_string(req.items.size() - 1) + "건" : "");
            pushData["items"] = pushItemsArray; // 🚀 한글 이름이 담긴 리스트!

            SessionManager::getInstance().sendToUser(ownerId, static_cast<uint16_t>(CmdID::NOTIFY_NEW_ORDER), pushData);
            std::cout << "[OrderHandler] 🔔 사장님(" << ownerId << ") 포스기에 알림 발송 완료!" << std::endl;
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

        // 1. 🚀 STORES 테이블에서 기존 예상 시간과 기본 조리 시간 가져오기 (JOIN 활용)
        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
            "SELECT S.delivery_time_range, S.cook_time "
            "FROM STORES S "
            "JOIN ORDERS O ON S.store_id = O.store_id "
            "WHERE O.order_id = ?"));
        pstmt->setString(1, req.orderId);
        std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());

        int netDeliveryTime = 15; // 기본 순수 배달 시간 (Fallback용)

        if (rs->next())
        {
            std::string timeRange = rs->isNull("delivery_time_range") ? "" : rs->getString("delivery_time_range").c_str();
            int defaultCookTime = rs->isNull("cook_time") ? 30 : rs->getInt("cook_time");

            // 2. 🚀 정규표현식으로 "20~30분" 등에서 가장 큰 숫자 추출
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

            // 3. 순수 배달 시간 계산 = (총 배달 예상 시간) - (기본 조리 시간)
            if (maxTotalTime > 0)
            {
                netDeliveryTime = maxTotalTime - defaultCookTime;
            }

            // 🛡️ [방어 로직] 계산 결과가 10분 미만이면 라이더 배달이 불가능하다고 판단, 최소 15분으로 보정
            if (netDeliveryTime < 10)
            {
                netDeliveryTime = 15;
            }

            std::cout << "[OrderHandler] 📊 분석: 기존 범위(" << timeRange << "), 기본 조리(" << defaultCookTime
                      << "분) -> 🛵 도출된 순수 배달시간: " << netDeliveryTime << "분" << std::endl;
        }

        // 4. 최종 고객 예상 시간 산출! (사장님 입력 조리시간 + 계산된 순수 배달시간)
        int totalEstimatedTime = req.cookTime + netDeliveryTime;

        // 5. 사장님(클라이언트)에게 성공 응답 쏘기 (3021)
        ResCookTimeSetDTO res = {200, "조리 시간이 성공적으로 설정되었습니다."};
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_COOK_TIME_SET), nlohmann::json(res));

        // 6. 고객 ID 조회 후 고객 앱으로 푸시 알림 (9010)
        std::string customerId = OrderDAO::getInstance().getCustomerIdByOrderId(req.orderId);
        if (!customerId.empty())
        {
            NotifyOrderStateDTO notifyCustomer;
            notifyCustomer.orderId = req.orderId;
            notifyCustomer.state = 1; // 1: 조리중 상태
            notifyCustomer.message = "사장님이 조리를 시작했습니다! (약 " + std::to_string(totalEstimatedTime) + "분 내 도착 예정 🛵)";

            SessionManager::getInstance().sendToUser(
                customerId,
                static_cast<uint16_t>(CmdID::NOTIFY_ORDER_STATE),
                nlohmann::json(notifyCustomer));
        }

        // 7. 라이더들에게 브로드캐스트 (9020) - 메뉴 요약에 조리시간 세팅
        NotifyDeliveryCallDTO notifyRiders;
        notifyRiders.orderId = req.orderId;
        notifyRiders.pickupAddress = StoreDAO::getInstance().getStoreDetail(OrderDAO::getInstance().getStoreIdByOrderId(req.orderId)).storeAddress;
        notifyRiders.deliveryAddress = OrderDAO::getInstance().getDeliveryAddressByOrderId(req.orderId);
        notifyRiders.deliveryFee = 3500;

        // 🚀 라이더에게 전달할 명확한 메시지
        notifyRiders.menuSummary = "조리 완료까지 " + std::to_string(req.cookTime) + "분 남음";

        int ROLE_RIDER = 2;
        SessionManager::getInstance().broadcastToRole(
            ROLE_RIDER,
            static_cast<uint16_t>(CmdID::NOTIFY_DELIVERY_CALL),
            nlohmann::json(notifyRiders));

        std::cout << "[OrderHandler] ✅ 고객 알림(" << totalEstimatedTime << "분 도착) 및 라이더 전파 완료" << std::endl;
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

        // 1. DB 업데이트 (상태 4: 배달 완료)
        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
            "UPDATE ORDERS SET order_status = 4 WHERE order_id = ?"));
        pstmt->setString(1, req.orderId);
        int affected = pstmt->executeUpdate();

        if (affected > 0)
        {
            // 2. 라이더에게 성공 응답 (4011)
            ResDeliveryCompleteDTO res = {200, "배달 완료 처리가 정상적으로 기록되었습니다."};
            session->sendPacket(static_cast<uint16_t>(CmdID::RES_DELIVERY_COMPLETE), nlohmann::json(res));

            // 3. 고객에게 알림 (9010 재활용)
            std::string customerId = OrderDAO::getInstance().getCustomerIdByOrderId(req.orderId);
            if (!customerId.empty())
            {
                NotifyOrderStateDTO notifyCustomer;
                notifyCustomer.orderId = req.orderId;
                notifyCustomer.state = 4; // 배달 완료 상태
                notifyCustomer.message = "배달이 완료되었습니다. 맛있게 드세요! 😋";

                SessionManager::getInstance().sendToUser(
                    customerId,
                    static_cast<uint16_t>(CmdID::NOTIFY_ORDER_STATE),
                    nlohmann::json(notifyCustomer));
            }

            // 4. 사장님에게 알림 (포스기 화면 갱신용)
            int storeId = OrderDAO::getInstance().getStoreIdByOrderId(req.orderId);
            std::string ownerId = StoreDAO::getInstance().getOwnerIdByStoreId(storeId);
            if (!ownerId.empty())
            {
                // 사장님도 9010 패킷을 수신하여 리스트에서 해당 주문을 '완료' 처리하도록 설계
                NotifyOrderStateDTO notifyOwner;
                notifyOwner.orderId = req.orderId;
                notifyOwner.state = 4;
                notifyOwner.message = "주문 번호[" + req.orderId + "] 배달이 완료되었습니다.";

                SessionManager::getInstance().sendToUser(
                    ownerId,
                    static_cast<uint16_t>(CmdID::NOTIFY_ORDER_STATE),
                    nlohmann::json(notifyOwner));
            }

            std::cout << "[OrderHandler] ✅ 배달 완료 및 고객/사장님 알림 전송 완료" << std::endl;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [OrderHandler] 배달 완료 처리 중 오류: " << e.what() << std::endl;
        ResDeliveryCompleteDTO res = {500, "서버 오류로 완료 처리에 실패했습니다."};
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_DELIVERY_COMPLETE), nlohmann::json(res));
    }
}
void OrderHandler::handleCheckoutInfo(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    auto req = nlohmann::json::parse(jsonBody).get<ReqCheckoutInfoDTO>();
    ResCheckoutInfoDTO res = {};
    res.status = 200;

    // 1. 배달비 퍼오기 (StoreDAO 활용)
    res.deliveryFee = StoreDAO::getInstance().getDeliveryFee(req.storeId);

    // 2. 🚀 유저 등급 퍼오기 (방금 만든 UserDAO 핀셋 함수 활용!)
    std::string currentGrade = UserDAO::getInstance().getUserGrade(req.userId);

    // 3. 비즈니스 로직: 와우 회원이면 배달비 0원!
    if (currentGrade == "wow" || currentGrade == "VIP")
    {
        res.deliveryFee = 0;
        std::cout << "[OrderHandler] 🎉 와우 회원 감지! 배달비 무료 적용." << std::endl;
    }

    session->sendPacket(static_cast<uint16_t>(CmdID::RES_CHECKOUT_INFO), nlohmann::json(res));
}
void OrderHandler::handleOrderReject(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        auto req = nlohmann::json::parse(jsonBody);
        std::string orderId = req.value("orderId", "");
        std::string reason = req.value("rejectReason", "재고 소진");

        auto conn = DBManager::getInstance().getConnection();
        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
            "UPDATE ORDERS SET order_status = 9 WHERE order_id = ?"));
        pstmt->setString(1, orderId);
        pstmt->executeUpdate();

        // 1. 사장님에게 성공 응답
        nlohmann::json res;
        res["status"] = 200;
        res["orderId"] = orderId;
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ORDER_REJECT), res);

        // 2. 고객에게 알림 전송 (9010번 활용)
        std::string customerId = OrderDAO::getInstance().getCustomerIdByOrderId(orderId);
        if (!customerId.empty())
        {
            nlohmann::json notify;
            notify["orderId"] = orderId;
            notify["state"] = 9;
            notify["message"] = "사장님이 주문을 거절하였습니다. 사유: " + reason;
            SessionManager::getInstance().sendToUser(customerId, static_cast<uint16_t>(CmdID::NOTIFY_ORDER_STATE), notify);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [OrderHandler] 주문 거절 처리 중 에러: " << e.what() << std::endl;
        nlohmann::json res;
        res["status"] = 500;
        res["message"] = "주문 거절 처리 중 서버 오류가 발생했습니다.";
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ORDER_REJECT), res);
    }
}

// [OrderHandler.cpp] 신규 추가
void OrderHandler::handleCancel(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        auto req = nlohmann::json::parse(jsonBody);
        std::string orderId = req.value("orderId", "");

        if (orderId.empty())
            throw std::runtime_error("주문 번호 누락");

        auto conn = DBManager::getInstance().getConnection();

        // 1. 주문 상태 업데이트 (9: 취소/거절)
        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
            "UPDATE ORDERS SET order_status = 9 WHERE order_id = ?"));
        pstmt->setString(1, orderId);
        int affected = pstmt->executeUpdate();

        if (affected > 0)
        {
            // 2. 관리자에게 성공 응답 (5012번 RES_CANCEL 가정)
            nlohmann::json res = {{"status", 200}, {"orderId", orderId}};
            session->sendPacket(static_cast<uint16_t>(CmdID::RES_CANCEL), res);

            // 3. 🚀 관련자 전원에게 실시간 푸시 전파 (9010번 활용)
            // 3-1. 고객에게 알림
            std::string customerId = OrderDAO::getInstance().getCustomerIdByOrderId(orderId);
            if (!customerId.empty())
            {
                nlohmann::json notify = {
                    {"orderId", orderId}, {"state", 9}, {"message", "관리자에 의해 주문이 취소/환불되었습니다."}};
                SessionManager::getInstance().sendToUser(customerId, static_cast<uint16_t>(CmdID::NOTIFY_ORDER_STATE), notify);
            }

            // 3-2. 사장님에게 알림 (포스기 리스트 갱신용)
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

void OrderHandler::handleOrderDetail(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        // 1. 요청 파싱 (단순 json으로 처리)
        auto req = nlohmann::json::parse(jsonBody);
        std::string orderId = req.value("orderId", "");

        if (orderId.empty())
        {
            throw std::runtime_error("주문 번호가 누락되었습니다.");
        }

        std::cout << "[OrderHandler] 🧾 영수증 상세 조회 요청 (OrderID: " << orderId << ")" << std::endl;

        auto conn = DBManager::getInstance().getConnection();
        ResOrderDetailDTO res;
        res.status = 200;
        res.orderId = orderId;

        // 🚀 2. ORDERS & STORES 조인: 영수증 헤더 긁어오기
        std::unique_ptr<sql::PreparedStatement> pstmtOrder(conn->prepareStatement(
            "SELECT S.store_name, O.created_at, O.delivery_address, O.total_price "
            "FROM ORDERS O "
            "JOIN STORES S ON O.store_id = S.store_id "
            "WHERE O.order_id = ?"));
        pstmtOrder->setString(1, orderId);
        std::unique_ptr<sql::ResultSet> rsOrder(pstmtOrder->executeQuery());

        if (rsOrder->next())
        {
            res.storeName = rsOrder->getString("store_name").c_str();
            res.createdAt = rsOrder->getString("created_at").c_str();
            res.deliveryAddress = rsOrder->getString("delivery_address").c_str();
            res.totalPrice = rsOrder->getInt("total_price");

            // 💡 아래 값들은 하드코딩 또는 DB 컬럼명에 맞게 수정하세요
            res.paymentMethod = "신용카드 결제";
            res.deliveryFee = 2800;
            res.couponDiscount = 0;
            res.wowDiscount = 0;
        }
        else
        {
            throw std::runtime_error("해당 주문을 찾을 수 없습니다.");
        }

        // 🚀 3. ORDER_DETAILS 조회: 태현님의 OrderItemDTO 규격에 완벽하게 맞춤!
        std::unique_ptr<sql::PreparedStatement> pstmtItems(conn->prepareStatement(
            "SELECT menu_id, menu_name, quantity, price, selected_options "
            "FROM ORDER_DETAILS WHERE order_id = ?"));
        pstmtItems->setString(1, orderId);
        std::unique_ptr<sql::ResultSet> rsItems(pstmtItems->executeQuery());

        int calculatedTotalMenuPrice = 0; // 순수 메뉴 합계 계산용

        while (rsItems->next())
        {
            OrderItemDTO item;

            // 💡 태현님의 DTO 필드명에 정확히 1:1 매핑!
            item.menuId = rsItems->getInt("menu_id");
            item.menuName = rsItems->getString("menu_name").c_str();
            item.quantity = rsItems->getInt("quantity");
            item.unitPrice = rsItems->getInt("price"); // DB의 price를 unitPrice에 담음

            // 순수 메뉴 총액 누적 (단가 * 수량)
            calculatedTotalMenuPrice += (item.unitPrice * item.quantity);

            // 🚀 핵심 포인트: DB에 저장된 '["매운맛", "단무지 추가"]' 문자열을 json 객체로 파싱해서 쏙!
            std::string optsStr = rsItems->isNull("selected_options") ? "[]" : rsItems->getString("selected_options").c_str();
            try
            {
                item.selectedOptions = nlohmann::json::parse(optsStr); // json 타입 변수에 바로 파싱!

                // 만약 파싱했는데 배열이 아니라면 빈 배열로 초기화 (방어 로직)
                if (!item.selectedOptions.is_array())
                {
                    item.selectedOptions = nlohmann::json::array();
                }
            }
            catch (...)
            {
                item.selectedOptions = nlohmann::json::array(); // 파싱 에러 시 빈 배열
            }

            res.items.push_back(item);
        }

        res.totalMenuPrice = calculatedTotalMenuPrice;
        nlohmann::json debugJson = res; // DTO를 JSON 객체로 자동 변환!
        std::cout << "\n================ [ 영수증 발급 대기 중 ] ================" << std::endl;
        std::cout << "[OrderHandler] 🧾 프론트엔드로 날아갈 2087번 패킷 데이터 최종 확인!" << std::endl;
        std::cout << debugJson.dump(4) << std::endl; // 💡 4칸 들여쓰기(dump(4))로 보기 좋게 출력!
        std::cout << "=========================================================\n"
                  << std::endl;

        // 4. 성공 응답 쏘기 (2087번)
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ORDER_DETAIL), res);
        std::cout << "[OrderHandler] ✅ 영수증 상세 데이터 전송 완료! (메뉴 " << res.items.size() << "개)" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [OrderHandler] 영수증 상세 조회 중 오류: " << e.what() << std::endl;
        ResOrderDetailDTO res;
        res.status = 500;
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ORDER_DETAIL), res);
    }
}