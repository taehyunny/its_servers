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
    result.status = 404; // 기본값: 찾지 못함

    try
    {
        auto conn = DBManager::getInstance().getConnection();

        // 🚀 [Fact Check] SQL 쿼리에 새로 추가된 3종 세트를 반드시 포함해야 합니다!
        std::unique_ptr<sql::PreparedStatement> pstmtStore(conn->prepareStatement(
            "SELECT S.store_id, S.store_name, S.store_address, S.open_time, S.close_time, S.delivery_fee, "
            "S.min_order_amount, S.rating, S.review_count, S.image_url, "
            "U.phone_number, U.user_name AS representative_name, O.business_number " // 👈 조인해서 가져올 신규 컬럼들
            "FROM STORES S "
            "LEFT JOIN USERS U ON S.owner_id = U.user_id "  // 사장님 전화번호, 이름 가져오기
            "LEFT JOIN OWNERS O ON S.owner_id = O.user_id " // 사장님 사업자번호 가져오기 (이전 에러 해결 반영!)
            "WHERE S.store_id = ?"));

        pstmtStore->setInt(1, storeId);
        std::unique_ptr<sql::ResultSet> rsStore(pstmtStore->executeQuery());

        if (rsStore->next())
        {
            result.status = 200;
            result.storeData.storeId = rsStore->getInt("store_id");
            result.storeData.storeName = rsStore->getString("store_name").c_str();
            result.storeData.storeAddress = rsStore->getString("store_address").c_str();
            result.storeData.operatingHours = rsStore->getString("open_time").c_str() + std::string(" ~ ") + rsStore->getString("close_time").c_str();
            result.storeData.deliveryFees = std::to_string(rsStore->getInt("delivery_fee")) + "원";
            result.storeData.isOpen = true; // 실제 로직에선 시간 체크 필요
            result.storeData.imageUrl = rsStore->isNull("image_url") ? "" : rsStore->getString("image_url").c_str();
            result.storeData.minOrderAmount = rsStore->getInt("min_order_amount");
            result.storeData.rating = rsStore->getDouble("rating");
            result.storeData.reviewCount = rsStore->getInt("review_count");
            result.storeData.deliveryTimeRange = "30~45분";

            // 🚀 [추가 데이터 3종] DB에서 꺼내서 DTO에 꽂기
            result.storeData.phoneNumber = rsStore->isNull("phone_number") ? "정보 없음" : rsStore->getString("phone_number").c_str();
            result.storeData.representativeName = rsStore->isNull("representative_name") ? "정보 없음" : rsStore->getString("representative_name").c_str();
            result.storeData.businessNumber = rsStore->isNull("business_number") ? "정보 없음" : rsStore->getString("business_number").c_str();
        }
        else
        {
            return result; // 매장이 없으면 리턴
        }
        // 🍔 2. 메뉴 리스트 조회
        std::unique_ptr<sql::PreparedStatement> pstmtMenu(conn->prepareStatement(
            "SELECT menu_id, menu_name, base_price, description, image_url, menu_category, is_sold_out, is_popular, menu_options "
            "FROM MENUS WHERE store_id = ?"));
        pstmtMenu->setInt(1, storeId);
        std::unique_ptr<sql::ResultSet> rsMenu(pstmtMenu->executeQuery());

        while (rsMenu->next())
        {
            MenuDataDTO menu;
            menu.menuId = rsMenu->getInt("menu_id");
            menu.storeId = storeId;
            menu.menuName = rsMenu->getString("menu_name").c_str();
            menu.basePrice = rsMenu->getInt("base_price");
            menu.isSoldOut = rsMenu->getBoolean("is_sold_out");
            menu.description = rsMenu->isNull("description") ? "" : rsMenu->getString("description").c_str();
            menu.imageUrl = rsMenu->isNull("image_url") ? "" : rsMenu->getString("image_url").c_str();
            menu.menuCategory = rsMenu->isNull("menu_category") ? "기본 메뉴" : rsMenu->getString("menu_category").c_str();
            menu.isPopular = rsMenu->getBoolean("is_popular");

            // JSON 옵션 파싱
            std::string optionsStr = rsMenu->isNull("menu_options") ? "[]" : rsMenu->getString("menu_options").c_str();
            try
            {
                menu.optionGroups = nlohmann::json::parse(optionsStr);
            }
            catch (...)
            {
                menu.optionGroups = nlohmann::json::array();
            }

            result.menuList.push_back(menu);
        }

        // ⭐ 3. 리뷰 리스트 조회
        std::unique_ptr<sql::PreparedStatement> pstmtReview(conn->prepareStatement(
            // 🚀 핵심: REVIEWS(R) 테이블과 MENUS(M) 테이블을 조인해서 메뉴 이름까지 가져옵니다!
            "SELECT R.review_id, R.order_id, R.user_id, R.store_id, R.menu_id, R.rating, "
            "R.content, R.image_url, R.owner_reply, R.created_at, M.menu_name "
            "FROM REVIEWS R "
            "LEFT JOIN MENUS M ON R.menu_id = M.menu_id "
            "WHERE R.store_id = ? "
            "ORDER BY R.created_at DESC"));

        pstmtReview->setInt(1, storeId); // 조회할 매장 번호
        std::unique_ptr<sql::ResultSet> rsReview(pstmtReview->executeQuery());

        while (rsReview->next())
        {
            ReviewDTO review;
            review.reviewId = rsReview->getInt("review_id");
            review.orderId = rsReview->getString("order_id").c_str();
            review.userId = rsReview->getString("user_id").c_str();
            review.storeId = rsReview->getInt("store_id");
            review.rating = rsReview->getInt("rating");
            review.content = rsReview->getString("content").c_str();

            // NULL 처리 (이미지나 사장님 댓글이 없을 수도 있으니까요!)
            review.imageUrl = rsReview->isNull("image_url") ? "" : rsReview->getString("image_url").c_str();
            review.ownerReply = rsReview->isNull("owner_reply") ? "" : rsReview->getString("owner_reply").c_str();
            review.createdAt = rsReview->getString("created_at").c_str();

            // 🚀 [사유하는 개발자의 디테일] DB에서 가져온 메뉴 정보 세팅
            review.menuId = rsReview->getInt("menu_id");
            // 만약 사장님이 메뉴를 삭제해서 MENUS 테이블에 없더라도, 리뷰는 보여야 하므로 방어 코드 작성!
            review.menuName = rsReview->isNull("menu_name") ? "삭제된 메뉴" : rsReview->getString("menu_name").c_str();

            result.reviewList.push_back(review);
        }
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "[StoreDAO] 매장 상세 조회 실패: " << e.what() << std::endl;
    }
    return result;
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