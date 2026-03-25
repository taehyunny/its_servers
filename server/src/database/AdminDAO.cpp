#include "AdminDAO.h"
#include "DbManager.h"
#include <iostream>
#include <mariadb/conncpp.hpp>

// =========================================================
// 👑 [관리자] 조건부 주문 전체 검색 로직
// =========================================================
std::vector<AdminOrderDTO> AdminDAO::searchOrders(const std::string &searchType, const std::string &keyword)
{
    std::vector<AdminOrderDTO> orderList;
    try
    {
        auto conn = DBManager::getInstance().getConnection();

        // 1. 기본 쿼리 작성 (관리자니까 가게 상관없이 전체를 봅니다)
        std::string query = "SELECT order_id, user_id, total_price, order_status, created_at "
                            "FROM ORDERS ";

        // 2. 🚀 동적 쿼리 조립: 검색어가 존재하는 경우에만 WHERE 절 추가
        if (!keyword.empty())
        {
            if (searchType == "orderId")
            {
                query += "WHERE order_id LIKE ? ";
            }
            else if (searchType == "userId")
            {
                query += "WHERE user_id LIKE ? ";
            }
        }

        // 최신 주문이 위로 오도록 정렬하고, 서버 과부하를 막기 위해 LIMIT 100을 겁니다.
        query += "ORDER BY created_at DESC LIMIT 100";

        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(query));

        // 3. 파라미터 바인딩 (LIKE 검색을 위해 앞뒤로 %를 붙여줍니다)
        if (!keyword.empty())
        {
            pstmt->setString(1, "%" + keyword + "%");
        }

        std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());

        // 4. 결과를 DTO에 매핑
        while (rs->next())
        {
            AdminOrderDTO order;
            order.orderId = rs->getString("order_id").c_str();
            order.userId = rs->getString("user_id").c_str();
            order.totalPrice = rs->getInt("total_price");
            order.orderStatus = rs->getInt("order_status");
            order.createdAt = rs->getString("created_at").c_str();

            orderList.push_back(order);
        }
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "🚨 [AdminDAO] 관리자 주문 검색 에러: " << e.what() << std::endl;
    }

    return orderList;
}