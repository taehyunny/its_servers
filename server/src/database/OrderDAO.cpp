#include "OrderDAO.h"
#include "DbManager.h"
#include <iostream>
#include <mariadb/conncpp.hpp>
#include <chrono> // 🚀 주문번호 생성을 위해 추가

// 🚀 밀리초 기반의 고유한 주문번호를 생성하는 헬퍼 함수
std::string generateOrderId()
{
    auto now = std::chrono::system_clock::now();                                                     // 현재 시간을 가져옵니다
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count(); // 1970년 1월 1일부터의 밀리초를 계산합니다
    return "ORD-" + std::to_string(ms);                                                              // 예: ORD-1710992345678
}

OrderResult OrderDAO::createOrder(const OrderCreateReqDTO &req) // 🚀 클라이언트의 주문 생성 요청을 DB에 저장하는 함수
{
    OrderResult result = {false, "", ""};                 // 초기 결과값 설정 (실패 상태)
    auto conn = DBManager::getInstance().getConnection(); // DB 연결 가져오기

    try
    {
        // 🚨 [방어벽] 수량 99개 제한 체크
        for (const auto &item : req.items)
        {
            if (item.quantity > 99 || item.quantity <= 0)
            {
                result.message = "메뉴 수량은 1개 이상 99개 이하만 가능합니다.";
                return result;
            }
        }

        // 🚀 서버에서 직접 주문번호 생성! (varchar(50) 사이즈에 딱 맞춤)
        std::string newOrderId = generateOrderId();

        // 🚀 트랜잭션 시작!
        conn->setAutoCommit(false);

        // ---------------------------------------------------------
        // 1. ORDERS 테이블에 부모 데이터 삽입 (스키마 완벽 대응!)
        // ---------------------------------------------------------
        // coupon_id 제거, order_status는 int형(0)으로 세팅
        std::string orderQuery = "INSERT INTO ORDERS (order_id, user_id, store_id, total_price, order_status, delivery_address) VALUES (?, ?, ?, ?, 0, ?)";
        std::unique_ptr<sql::PreparedStatement> pstmtOrder(conn->prepareStatement(orderQuery)); // 🚀 PreparedStatement로 SQL 인젝션 방어!

        pstmtOrder->setString(1, newOrderId); // 우리가 만든 주문번호 삽입!
        pstmtOrder->setString(2, req.userId);
        pstmtOrder->setInt(3, req.storeId);
        pstmtOrder->setInt(4, req.totalPrice);
        pstmtOrder->setString(5, req.deliveryAddress);

        pstmtOrder->executeUpdate();

        // ---------------------------------------------------------
        // 2. ORDER_ITEMS 테이블에 메뉴들 쫙 삽입
        // ---------------------------------------------------------
        std::string itemQuery = "INSERT INTO ORDER_ITEMS (order_id, menu_id, quantity, unit_price, selected_options) VALUES (?, ?, ?, ?, ?)";
        std::unique_ptr<sql::PreparedStatement> pstmtItem(conn->prepareStatement(itemQuery)); // 🚀 PreparedStatement로 SQL 인젝션 방어!

        for (const auto &item : req.items)
        {
            pstmtItem->setString(1, newOrderId); // 위에서 만든 동일한 주문번호 사용
            pstmtItem->setInt(2, item.menuId);
            pstmtItem->setInt(3, item.quantity);
            pstmtItem->setInt(4, item.unitPrice);

            // JSON 스냅샷 문자열 변환
            pstmtItem->setString(5, item.selectedOptions.dump());

            pstmtItem->executeUpdate();
        }

        // 🚀 모든 삽입 완료. 확정!
        conn->commit();
        conn->setAutoCommit(true);

        result.isSuccess = true;
        result.message = "주문이 성공적으로 생성되었습니다.";
        result.orderId = newOrderId; // 클라이언트에게 생성된 주문번호 전달

        std::cout << "[OrderDAO] 🟢 주문 생성 완료! (주문번호: " << newOrderId << ")" << std::endl;
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "[OrderDAO] 🔴 DB 에러! 롤백합니다: " << e.what() << std::endl;
        try
        {
            conn->rollback();
            conn->setAutoCommit(true);
        }
        catch (sql::SQLException &re)
        {
            std::cerr << "[OrderDAO] 롤백 실패: " << re.what() << std::endl;
        }

        result.isSuccess = false;
        result.message = "서버 오류로 주문에 실패했습니다.";
    }

    return result;
}

// 1. 주문 상태 업데이트
bool OrderDAO::updateOrderStatus(const std::string &orderId, int status)
{
    try
    {
        auto conn = DBManager::getInstance().getConnection();
        // 🚀 스키마에 맞춰 order_status 컬럼 업데이트
        std::string query = "UPDATE ORDERS SET order_status = ? WHERE order_id = ?";

        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(query));
        pstmt->setInt(1, status);
        pstmt->setString(2, orderId);

        return pstmt->executeUpdate() > 0; // 업데이트 성공 시 true
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "[OrderDAO] 상태 업데이트 실패: " << e.what() << std::endl;
        return false;
    }
}

// 2. 주문번호로 고객(user_id) 찾기
std::string OrderDAO::getCustomerIdByOrderId(const std::string &orderId)
{
    try
    {
        auto conn = DBManager::getInstance().getConnection();
        std::string query = "SELECT user_id FROM ORDERS WHERE order_id = ?";

        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(query));
        pstmt->setString(1, orderId);

        std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());
        if (rs->next())
        {
            return rs->getString("user_id").c_str();
        }
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "[OrderDAO] 고객 ID 조회 실패: " << e.what() << std::endl;
    }
    return ""; // 못 찾으면 빈 문자열
}

std::vector<OrderHistoryItemDTO> OrderDAO::getOrderHistory(const std::string &userId, const std::string &keyword)
{
    std::vector<OrderHistoryItemDTO> result;
    try
    {
        auto conn = DBManager::getInstance().getConnection();

        // 🚀 1. 쿼리 수정: order_status IN (4, -1) 조건 삭제!
        // 이제 대기 중(0), 조리 중(1), 배달 중(3)인 모든 주문이 화면에 나옵니다.
        std::string query =
            "SELECT O.order_id, O.store_id, S.store_name, O.total_price, "
            "O.order_status, O.created_at "
            "FROM ORDERS O "
            "JOIN STORES S ON O.store_id = S.store_id "
            "WHERE O.user_id = ? "; // 💡 여기가 핵심 수정 포인트!

        // 검색어(keyword)가 있으면 가게 이름으로 필터링
        if (!keyword.empty())
        {
            query += "AND S.store_name LIKE ? ";
        }
        query += "ORDER BY O.created_at DESC";

        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(query));
        pstmt->setString(1, userId);

        if (!keyword.empty())
        {
            pstmt->setString(2, "%" + keyword + "%");
        }

        std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());

        while (rs->next())
        {
            OrderHistoryItemDTO item;

            // 🚀 2. DTO 규격에 맞춘 완벽한 매핑
            item.orderId = rs->getString("order_id").c_str();
            item.storeId = rs->getInt("store_id");
            item.storeName = rs->getString("store_name").c_str();
            item.totalPrice = rs->getInt("total_price");
            item.status = rs->getInt("order_status"); // 💡 프론트엔드가 이걸 보고 상태를 표시합니다
            item.createdAt = rs->getString("created_at").c_str();

            // (배달 사진은 현재 DB 컬럼에 없다고 가정하고 빈 문자열 처리)
            item.deliveryPhotoUrl = "";

            // 🚀 3. 메뉴 요약(menuSummary) 만들기
            // 💡 주의: 여기서 테이블 이름은 복수형(ORDER_ITEMS)이 맞습니다!
            std::unique_ptr<sql::PreparedStatement> pstmtMenu(conn->prepareStatement(
                "SELECT M.menu_name "
                "FROM ORDER_ITEMS OI "                    // 💡 테이블 이름 S 확인!
                "JOIN MENUS M ON OI.menu_id = M.menu_id " // 💡 JOIN 포인트
                "WHERE OI.order_id = ?"));

            pstmtMenu->setString(1, item.orderId);
            std::unique_ptr<sql::ResultSet> rsMenu(pstmtMenu->executeQuery());

            std::string firstMenu = "";
            int menuCount = 0;

            while (rsMenu->next())
            {
                if (menuCount == 0)
                    firstMenu = std::string(rsMenu->getString("menu_name"));
                menuCount++;
            }
            item.menuName = firstMenu.empty() ? "메뉴 정보 없음" : firstMenu;

            if (menuCount > 1)
            {
                item.menuSummary = firstMenu + " 외 " + std::to_string(menuCount - 1) + "건";
            }
            else if (menuCount == 1)
            {
                item.menuSummary = firstMenu;
            }
            else
            {
                item.menuSummary = "메뉴 정보 없음";
            }

            result.push_back(item);
        }
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "🚨 [OrderDAO] 과거 주문 내역 조회 에러: " << e.what() << std::endl;
    }
    return result;
}

// 1. 주문번호로 매장 ID 가져오기
int OrderDAO::getStoreIdByOrderId(const std::string &orderId)
{
    auto conn = DBManager::getInstance().getConnection();
    std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
        "SELECT store_id FROM ORDERS WHERE order_id = ?"));
    pstmt->setString(1, orderId);
    std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());
    if (rs->next())
        return rs->getInt("store_id");
    return -1;
}

// 2. 주문번호로 배달 주소 가져오기
std::string OrderDAO::getDeliveryAddressByOrderId(const std::string &orderId)
{
    auto conn = DBManager::getInstance().getConnection();
    std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
        "SELECT delivery_address FROM ORDERS WHERE order_id = ?"));
    pstmt->setString(1, orderId);
    std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());
    if (rs->next())
        return rs->getString("delivery_address").c_str();
    return "";
}