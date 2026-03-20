#include "StoreDAO.h"
#include "DbManager.h" // ⚠️ 태현님 프로젝트의 실제 파일명에 맞게 대소문자 확인
#include <iostream>
#include <mariadb/conncpp.hpp>

std::vector<TopStoreInfo> StoreDAO::getAllStores()
{
    std::vector<TopStoreInfo> topStores;
    try
    {
        auto conn = DBManager::getInstance().getConnection();

        // 🚀 JOIN을 써서 CATEGORIES 테이블의 icon_name을 바로 가져옵니다!
        std::string query = R"(
            SELECT S.store_id, S.store_name, S.category, S.total_sales, C.icon_name
            FROM (
                SELECT store_id, store_name, category, total_sales,
                       ROW_NUMBER() OVER (PARTITION BY category ORDER BY total_sales DESC) as rnk
                FROM STORES
            ) as S
            JOIN CATEGORIES C ON S.category = C.name
            WHERE S.rnk = 1
        )";

        std::unique_ptr<sql::Statement> stmt(conn->createStatement());
        std::unique_ptr<sql::ResultSet> rs(stmt->executeQuery(query));

        while (rs->next())
        {
            TopStoreInfo store;
            store.storeId = rs->getInt("store_id");
            store.storeName = rs->getString("store_name");
            store.category = rs->getString("category");
            store.totalSales = rs->getInt("total_sales");
            // 🚀 DB에서 가져온 값을 그대로 꽂아주면 끝! (동적 구현)
            store.iconPath = rs->getString("icon_name");

            topStores.push_back(store);
        }
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "[StoreDAO] 메인 화면 조회 실패: " << e.what() << std::endl;
    }
    return topStores;
}
std::vector<TopStoreInfo> StoreDAO::getStoresByCategory(const std::string &categoryName)
{
    std::vector<TopStoreInfo> stores;
    try
    {
        auto conn = DBManager::getInstance().getConnection();

        // 🚀 선택된 카테고리의 매장만 매출 1등부터 꼴등까지 쫙 가져옵니다!
        std::string query = "SELECT store_id, store_name, category, total_sales FROM STORES WHERE category = ? ORDER BY total_sales DESC";
        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(query));
        pstmt->setString(1, categoryName);

        std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());

        while (rs->next())
        {
            TopStoreInfo store;
            store.storeId = rs->getInt("store_id");
            store.storeName = rs->getString("store_name");
            store.category = rs->getString("category");
            store.totalSales = rs->getInt("total_sales");
            // 아이콘은 이미 메인 화면에서 보여줬으므로 여기선 생략해도 무방합니다.
            stores.push_back(store);
        }
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "[StoreDAO] 카테고리별 매장 리스트 조회 실패: " << e.what() << std::endl;
    }
    return stores;
}