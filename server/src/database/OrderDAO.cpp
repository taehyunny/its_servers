#include "OrderDAO.h"
#include "DbManager.h"
#include <iostream>
#include <mariadb/conncpp.hpp>
#include <chrono> // 🚀 주문번호 생성을 위해 추가

// 🚀 밀리초 기반의 고유한 주문번호를 생성하는 헬퍼 함수
std::string generateOrderId()
{
    auto now = std::chrono::system_clock::now();  // 현재 시간을 가져옵니다
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count(); // 1970년 1월 1일부터의 밀리초를 계산합니다
    return "ORD-" + std::to_string(ms); // 예: ORD-1710992345678
}

OrderResult OrderDAO::createOrder(const OrderCreateReqDTO &req) // 🚀 클라이언트의 주문 생성 요청을 DB에 저장하는 함수
{
    OrderResult result = {false, "", ""};  // 초기 결과값 설정 (실패 상태)
    auto conn = DBManager::getInstance().getConnection();   // DB 연결 가져오기

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