#include "StoreDAO.h"
#include "DbManager.h"
#include <iostream>
#include <mariadb/conncpp.hpp>

// ---------------------------------------------------------
// 1. 메인 화면용: 각 카테고리별 매출 1등 매장들만 가져오기
// ---------------------------------------------------------
std::vector<TopStoreInfo> StoreDAO::getTopStoresByCategory()
{
    std::vector<TopStoreInfo> topStores;
    try
    {
        auto conn = DBManager::getInstance().getConnection();

        // 🚀 새로 추가된 컬럼들을 모조리 SELECT 합니다! (내부적으로 total_sales로 1등 계산)
        std::string query = R"(
            SELECT S.store_id, S.store_name, S.category, C.icon_name,
                   S.delivery_time_range, S.rating, S.review_count, S.min_order_amount, S.delivery_fee
            FROM (
                SELECT store_id, store_name, category, total_sales,
                       delivery_time_range, rating, review_count, min_order_amount, delivery_fee,
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
            store.iconPath = rs->getString("icon_name");

            // 🚀 새로운 필드들 파싱! (DB 컬럼명이 다르면 여기서 맞춰주세요)
            store.deliveryTimeRange = rs->getString("delivery_time_range");
            store.rating = rs->getDouble("rating");
            store.reviewCount = rs->getInt("review_count");
            store.minOrderAmount = rs->getInt("min_order_amount");
            store.deliveryFee = rs->getInt("delivery_fee");

            topStores.push_back(store);
        }
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "[StoreDAO] 메인 화면 1등 매장 조회 실패: " << e.what() << std::endl;
    }
    return topStores;
}

// ---------------------------------------------------------
// 2. 특정 카테고리의 전체 매장 추출 (상세 리스트용)
// ---------------------------------------------------------
std::vector<TopStoreInfo> StoreDAO::getStoresByCategory(const std::string &categoryName) {
    std::vector<TopStoreInfo> stores;
    try {
        auto conn = DBManager::getInstance().getConnection();
        
        // 🚀 여기도 JOIN을 걸어서 icon_name까지 예쁘게 다 채워줍니다!
        std::string query = R"(
            SELECT S.store_id, S.store_name, S.category, C.icon_name,
                   S.delivery_time_range, S.rating, S.review_count, S.min_order_amount, S.delivery_fee
            FROM STORES S
            JOIN CATEGORIES C ON S.category = C.name
            WHERE S.category = ?
            ORDER BY S.total_sales DESC
        )";
        
        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(query));
        pstmt->setString(1, categoryName);
        std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());

        while (rs->next()) {
            TopStoreInfo store;
            store.storeId = rs->getInt("store_id");
            store.storeName = rs->getString("store_name");
            store.category = rs->getString("category");
            store.iconPath = rs->getString("icon_name");
            
            // 🚀 새로운 필드들 파싱!
            store.deliveryTimeRange = rs->getString("delivery_time_range");
            store.rating = rs->getDouble("rating");
            store.reviewCount = rs->getInt("review_count");
            store.minOrderAmount = rs->getInt("min_order_amount");
            store.deliveryFee = rs->getInt("delivery_fee");

            stores.push_back(store);
        }
    } catch (sql::SQLException &e) {
        std::cerr << "[StoreDAO] 카테고리별 매장 리스트 조회 실패: " << e.what() << std::endl;
    }
    return stores;
}
std::vector<TopStoreInfo> StoreDAO::getAllStores()
{
    std::vector<TopStoreInfo> stores;
    try
    {
        auto conn = DBManager::getInstance().getConnection();

        // 카테고리 상관없이 모든 매장을 매출순으로 가져오는 쿼리
        std::string query = "SELECT store_id, store_name, category, total_sales FROM STORES ORDER BY total_sales DESC";
        std::unique_ptr<sql::Statement> stmt(conn->createStatement());
        std::unique_ptr<sql::ResultSet> rs(stmt->executeQuery(query));

        while (rs->next())
        {
            TopStoreInfo store;
            store.storeId = rs->getInt("store_id");
            store.storeName = rs->getString("store_name");
            store.category = rs->getString("category");
            store.iconPath = rs->getString("icon_name");
            
            // 🚀 새로운 필드들 파싱!
            store.deliveryTimeRange = rs->getString("delivery_time_range");
            store.rating = rs->getDouble("rating");
            store.reviewCount = rs->getInt("review_count");
            store.minOrderAmount = rs->getInt("min_order_amount");
            store.deliveryFee = rs->getInt("delivery_fee");
            stores.push_back(store);
        }
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "[StoreDAO] 전체 매장 조회 실패: " << e.what() << std::endl;
    }
    return stores;
}