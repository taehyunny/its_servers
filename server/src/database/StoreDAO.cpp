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
            ORDER BY S.total_sales DESC -- 🚀 범인 검거! 최종 결과도 매출순으로 줄을 세웁니다.
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
std::vector<TopStoreInfo> StoreDAO::getStoresByCategoryId(int categoryId)
{
    std::vector<TopStoreInfo> stores;
    try
    {
        auto conn = DBManager::getInstance().getConnection();

        // 🚀 [핵심] CATEGORIES 테이블과 JOIN해서 category_id로 필터링합니다!
        std::string query = R"(
            SELECT S.store_id, S.store_name, S.category, C.icon_name,
                   S.delivery_time_range, S.rating, S.review_count, S.min_order_amount, S.delivery_fee
            FROM STORES S
            JOIN CATEGORIES C ON S.category = C.name
            WHERE C.category_id = ?
            ORDER BY S.total_sales DESC
        )";

        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(query));
        pstmt->setInt(1, categoryId); // 🎯 클라이언트가 보낸 숫자(2, 3 등)를 여기에 꽂습니다.

        std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());

        while (rs->next())
        {
            TopStoreInfo store;
            store.storeId = rs->getInt("store_id");
            store.storeName = rs->getString("store_name");
            store.category = rs->getString("category");
            store.iconPath = rs->getString("icon_name");

            store.deliveryTimeRange = rs->getString("delivery_time_range");
            store.rating = rs->getDouble("rating");
            store.reviewCount = rs->getInt("review_count");
            store.minOrderAmount = rs->getInt("min_order_amount");
            store.deliveryFee = rs->getInt("delivery_fee");

            stores.push_back(store);
        }
        std::cout << "[StoreDAO] 카테고리 ID " << categoryId << " 상점 " << stores.size() << "개 로드 완료." << std::endl;
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "[StoreDAO] 카테고리 ID 기반 조회 실패: " << e.what() << std::endl;
    }
    return stores;
}
std::vector<TopStoreInfo> StoreDAO::getAllStores()
{
    std::vector<TopStoreInfo> stores;
    try
    {
        auto conn = DBManager::getInstance().getConnection();

        // ✅ [수정] 쿼리에 필요한 모든 컬럼을 넣고, icon_name을 위해 CATEGORIES와 JOIN 합니다!
        std::string query = R"(
            SELECT S.store_id, S.store_name, S.category, C.icon_name,
                   S.delivery_time_range, S.rating, S.review_count, S.min_order_amount, S.delivery_fee
            FROM STORES S
            JOIN CATEGORIES C ON S.category = C.name
            ORDER BY S.total_sales DESC
        )";

        std::unique_ptr<sql::Statement> stmt(conn->createStatement());
        std::unique_ptr<sql::ResultSet> rs(stmt->executeQuery(query));

        while (rs->next())
        {
            TopStoreInfo store;
            store.storeId = rs->getInt("store_id");
            store.storeName = rs->getString("store_name");
            store.category = rs->getString("category");
            store.iconPath = rs->getString("icon_name"); // 이제 에러 안 남!

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

// =========================================================
ResStoreDetailDTO StoreDAO::getStoreDetail(int storeId)
{
    ResStoreDetailDTO result;
    result.status = 404; // 기본값: 매장을 찾을 수 없음 (에러 대비)

    try
    {
        auto conn = DBManager::getInstance().getConnection();

        // ---------------------------------------------------------
        // 📦 1단계: 상점 기본 정보 조회 (STORES 테이블)
        // ---------------------------------------------------------
        std::unique_ptr<sql::PreparedStatement> pstmtStore(conn->prepareStatement(
            // 🚀 icon_path를 image_url로 수정!
            "SELECT store_id, store_name, store_address, open_time, close_time, delivery_fee, "
            "min_order_amount, rating, review_count, image_url "
            "FROM STORES WHERE store_id = ?"));
        pstmtStore->setInt(1, storeId);
        std::unique_ptr<sql::ResultSet> rsStore(pstmtStore->executeQuery());

        if (rsStore->next())
        {
            result.status = 200; // 매장을 찾았으니 200 OK!
            result.storeData.store_id = rsStore->getInt("store_id");
            result.storeData.store_name = rsStore->getString("store_name").c_str();
            result.storeData.store_address = rsStore->getString("store_address").c_str();

            // 🚀 가공 포인트 1: 영업시간 합치기 ("09:00 ~ 21:00")
            result.storeData.operating_hours = rsStore->getString("open_time").c_str() + std::string(" ~ ") + rsStore->getString("close_time").c_str();

            // 🚀 가공 포인트 2: 배달비 문자열로 만들기 (프론트엔드 팀원 요청: "3000원")
            result.storeData.delivery_fees = std::to_string(rsStore->getInt("delivery_fee")) + "원";

            result.storeData.is_open = true; // (TODO: TimeUtil을 써서 현재 시간이 영업시간 안인지 체크하는 로직 추가 가능)
            result.storeData.image_url = rsStore->isNull("image_url") ? "" : rsStore->getString("image_url").c_str();
            result.storeData.min_order_amount = rsStore->getInt("min_order_amount");
            result.storeData.rating = rsStore->getDouble("rating");
            result.storeData.review_count = rsStore->getInt("review_count");

            // (TODO: 배달 예상 시간은 DB 컬럼에 맞게 수정 필요)
            result.storeData.delivery_time_range = "30~40분";
        }
        else
        {
            // 매장이 없으면 메뉴나 리뷰를 캘 필요도 없이 바로 리턴!
            return result;
        }

        // ---------------------------------------------------------
        // 🍔 2단계: 해당 매장의 메뉴 리스트 싹쓸이 (MENUS 테이블)
        // ---------------------------------------------------------
        std::unique_ptr<sql::PreparedStatement> pstmtMenu(conn->prepareStatement(
            // 🚀 스키마 완벽 반영: menu_name, base_price, image_url, menu_category
            "SELECT menu_id, menu_name, base_price, description, image_url, menu_category, is_sold_out, is_popular "
            "FROM MENUS WHERE store_id = ?"));
        pstmtMenu->setInt(1, storeId);
        std::unique_ptr<sql::ResultSet> rsMenu(pstmtMenu->executeQuery());

        while (rsMenu->next())
        {
            MenuDataDTO menu;
            menu.menu_id = rsMenu->getInt("menu_id");
            menu.store_id = storeId;

            // 🚀 꺼낼 때도 정확한 컬럼명으로!
            menu.menu_name = rsMenu->getString("menu_name").c_str();
            menu.base_price = rsMenu->getInt("base_price");
            menu.is_sold_out = rsMenu->getBoolean("is_sold_out");
            menu.description = rsMenu->isNull("description") ? "" : rsMenu->getString("description").c_str();
            menu.image_url = rsMenu->isNull("image_url") ? "" : rsMenu->getString("image_url").c_str();
            menu.menu_category = rsMenu->isNull("menu_category") ? "기본 메뉴" : rsMenu->getString("menu_category").c_str();
            menu.is_popular = rsMenu->getBoolean("is_popular");

            result.menuList.push_back(menu);
        }
        // ---------------------------------------------------------
        // ⭐ 3단계: 해당 매장의 리뷰 리스트 싹쓸이 (REVIEWS 테이블)
        // ---------------------------------------------------------
        std::unique_ptr<sql::PreparedStatement> pstmtReview(conn->prepareStatement(
            "SELECT review_id, user_id, order_id, rating, content, created_at "
            "FROM REVIEWS WHERE store_id = ? ORDER BY created_at DESC"));
        pstmtReview->setInt(1, storeId);
        std::unique_ptr<sql::ResultSet> rsReview(pstmtReview->executeQuery());

        while (rsReview->next())
        {
            ReviewDataDTO review;
            review.review_id = rsReview->getInt("review_id");
            review.store_id = storeId;
            review.user_id = rsReview->getString("user_id").c_str();
            review.order_id = rsReview->getInt("order_id");
            review.rating = rsReview->getInt("rating");
            review.content = rsReview->getString("content").c_str();
            review.created_at = rsReview->getString("created_at").c_str();

            result.reviewList.push_back(review); // 리뷰 상자에 담기
        }
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "[StoreDAO] 매장 상세 정보 조회 DB 에러: " << e.what() << std::endl;
        result.status = 500; // 서버 에러 발생!
    }

    return result; // 꽉 채운 마스터 DTO 반환!
}

bool StoreDAO::executeUpdate(const std::string &query, const std::vector<std::string> &params)
{
    try
    {
        auto conn = DBManager::getInstance().getConnection();
        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(query));

        // ? 개수만큼 파라미터를 바인딩합니다.
        for (size_t i = 0; i < params.size(); ++i)
        {
            pstmt->setString(i + 1, params[i]);
        }

        pstmt->executeUpdate();
        return true;
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "[StoreDAO] 동적 업데이트 실패: " << e.what() << std::endl;
        return false;
    }
}