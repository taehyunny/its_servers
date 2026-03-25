#include "SearchDAO.h"
#include "DbManager.h"
#include <iostream>

// =========================================================
// 1. 검색 위젯 초기화 (인기 카테고리 & 최근 검색어)
// =========================================================

std::vector<PopularKeyword> SearchDAO::getPopularCategories(int limit)
{
    std::vector<PopularKeyword> result;
    try
    {
        auto conn = DBManager::getInstance().getConnection();

        // 🚀 핵심: 복잡한 조인을 버리고, 이미 점수가 매겨진 CATEGORIES 테이블에서 1등부터 줄을 세웁니다!
        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
            "SELECT name AS category_name, popularity_score "
            "FROM CATEGORIES "
            "ORDER BY popularity_score DESC "
            "LIMIT ?"));

        pstmt->setInt(1, limit);
        std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());

        int currentRank = 1;
        while (rs->next())
        {
            PopularKeyword pk;
            pk.rank = currentRank++;

            // 🚀 DTO의 'keyword' 변수에 진짜 카테고리 이름(예: 분식, 중식)을 담아서 프론트에 보냅니다.
            pk.keyword = rs->getString("category_name").c_str();

            result.push_back(pk);
        }
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "[SearchDAO] 인기 카테고리 조회 실패: " << e.what() << std::endl;
    }
    return result;
}

// =========================================================
// 2. 삼위일체 로직 (기록 저장 & 점수 반영)
// =========================================================

bool SearchDAO::insertSearchHistory(const std::string &userId, const std::string &keyword)
{
    try
    {
        auto conn = DBManager::getInstance().getConnection();
        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
            "INSERT INTO SEARCH_HISTORY (user_id, keyword, is_visible) VALUES (?, ?, 1)"));
        pstmt->setString(1, userId);
        pstmt->setString(2, keyword);
        pstmt->executeUpdate();
        return true;
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "[SearchDAO] 검색어 저장 실패: " << e.what() << std::endl;
        return false;
    }
}

bool SearchDAO::applySearchScore(const std::string &keyword)
{
    try
    {
        auto conn = DBManager::getInstance().getConnection();

        // 🚀 [추가된 0순위]: 검색어 자체가 카테고리명(예: "치킨", "분식")인지 먼저 확인!
        std::unique_ptr<sql::PreparedStatement> pstmtCatDirect(conn->prepareStatement(
            "UPDATE CATEGORIES SET popularity_score = popularity_score + 1 WHERE name = ?"));
        pstmtCatDirect->setString(1, keyword);
        int directCatUpdated = pstmtCatDirect->executeUpdate();

        if (directCatUpdated > 0)
        {
            std::cout << "[SearchDAO] 카테고리 다이렉트 일치! 점수 +1 (" << keyword << ")" << std::endl;
            return true; // 여기서 성공했으니 끝냅니다!
        }

        // 1순위: 완벽한 매장명 검색
        std::unique_ptr<sql::PreparedStatement> pstmtStore(conn->prepareStatement(
            "UPDATE STORES SET popularity_score = popularity_score + 1 WHERE store_name = ?"));
        pstmtStore->setString(1, keyword);
        int rowsUpdated = pstmtStore->executeUpdate();

        if (rowsUpdated > 0)
        {
            std::cout << "[SearchDAO] 매장명 일치! 매장 점수 +1 (" << keyword << ")" << std::endl;
            return true;
        }

        // 2 & 3순위: 메뉴 검색 (기존 로직 그대로 유지)
        std::unique_ptr<sql::PreparedStatement> pstmtCategory(conn->prepareStatement(
            "UPDATE CATEGORIES SET popularity_score = popularity_score + 1 "
            "WHERE name = ("
            "    SELECT S.category FROM MENUS M "
            "    JOIN STORES S ON M.store_id = S.store_id "
            "    WHERE M.menu_name LIKE ? LIMIT 1"
            ")"));

        std::string searchPattern = "%" + keyword + "%";
        pstmtCategory->setString(1, searchPattern);
        int catUpdated = pstmtCategory->executeUpdate();

        if (catUpdated > 0)
        {
            std::cout << "[SearchDAO] 메뉴명(부분) 일치! 카테고리 점수 +1 (" << keyword << ")" << std::endl;
        }
        else
        {
            std::cout << "[SearchDAO] 검색어에 해당하는 매장/메뉴/카테고리가 없습니다. (" << keyword << ")" << std::endl;
        }

        return true;
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "[SearchDAO] 검색 점수 반영 실패: " << e.what() << std::endl;
        return false;
    }
}

// =========================================================
// 3. 실시간 매장 검색 (LIKE 검색)
// =========================================================

std::vector<TopStoreInfo> SearchDAO::searchStoresByKeyword(const std::string &keyword)
{
    std::vector<TopStoreInfo> result;
    try
    {
        auto conn = DBManager::getInstance().getConnection();

        // 🚀 1. DB 스키마에 완벽하게 맞춘 SELECT 쿼리 (가게 이름 OR 카테고리 검색)
        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
            "SELECT store_id, store_name, category, rating, review_count, "
            "min_order_amount, delivery_time_range, delivery_fee, image_url "
            "FROM STORES "
            "WHERE store_name LIKE ? OR category LIKE ?"));

        std::string searchPattern = "%" + keyword + "%";
        pstmt->setString(1, searchPattern);
        pstmt->setString(2, searchPattern); // 카테고리 검색용

        std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());

        // 🚀 2. DB 컬럼명으로 꺼내서 DTO에 매핑하기
        while (rs->next())
        {
            TopStoreInfo store;
            store.storeId = rs->getInt("store_id");
            store.storeName = rs->getString("store_name").c_str(); // name -> store_name
            store.category = rs->getString("category").c_str();

            // NULL 체크를 가미한 안전한 데이터 추출
            store.rating = rs->isNull("rating") ? 0.0 : rs->getDouble("rating");
            store.reviewCount = rs->isNull("review_count") ? 0 : rs->getInt("review_count");

            // DTO 변수명(Price)과 DB 컬럼명(amount) 매핑!
            store.minOrderPrice = rs->isNull("min_order_amount") ? 0 : rs->getInt("min_order_amount");
            store.deliveryTime = rs->isNull("delivery_time_range") ? "" : rs->getString("delivery_time_range").c_str();
            store.deliveryFee = rs->isNull("delivery_fee") ? 0 : rs->getInt("delivery_fee");
            store.iconPath = rs->isNull("image_url") ? "" : rs->getString("image_url").c_str();

            result.push_back(store);
        }
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "[SearchDAO] 매장 검색 실패: " << e.what() << std::endl;
    }
    return result;
}
// =========================================================
// 4. 최근 검색어 삭제 로직
// =========================================================

bool SearchDAO::deleteRecentSearch(const std::string &userId, int historyId)
{
    try
    {
        auto conn = DBManager::getInstance().getConnection();
        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
            "UPDATE SEARCH_HISTORY SET is_visible = 0 WHERE user_id = ? AND history_id = ?"));
        pstmt->setString(1, userId);
        pstmt->setInt(2, historyId);
        pstmt->executeUpdate();
        std::cout << "[SearchDAO] 최근 검색어 삭제 완료 (History ID: " << historyId << ")" << std::endl;
        return true;
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "[SearchDAO] 최근 검색어 삭제 에러: " << e.what() << std::endl;
        return false;
    }
}

bool SearchDAO::deleteAllRecentSearches(const std::string &userId)
{
    try
    {
        auto conn = DBManager::getInstance().getConnection();
        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
            "UPDATE SEARCH_HISTORY SET is_visible = 0 WHERE user_id = ?"));
        pstmt->setString(1, userId);
        pstmt->executeUpdate();
        return true;
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "[SearchDAO] 최근 검색어 전체 삭제 실패: " << e.what() << std::endl;
        return false;
    }
}

std::vector<RecentSearch> SearchDAO::getRecentSearches(const std::string &userId)
{
    std::vector<RecentSearch> result;
    try
    {
        auto conn = DBManager::getInstance().getConnection();

        // 🚀 DATE_FORMAT을 사용해 '월.일' (예: 03.23) 포맷으로 변환해서 가져옵니다!
        std::string query = R"(
            SELECT history_id, keyword, DATE_FORMAT(search_date, '%m.%d') as formatted_date 
            FROM SEARCH_HISTORY 
            WHERE user_id = ? AND is_visible = 1 
            ORDER BY search_date DESC 
            LIMIT 10
        )";

        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(query));
        pstmt->setString(1, userId);
        std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());

        while (rs->next())
        {
            RecentSearch rsItem;
            rsItem.historyId = rs->getInt("history_id");
            rsItem.keyword = rs->getString("keyword").c_str();

            // 🚀 DB에서 예쁘게 자른 날짜를 DTO에 담아줍니다.
            rsItem.searchDate = rs->getString("formatted_date").c_str();

            result.push_back(rsItem);
        }
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "[SearchDAO] 최근 검색어 조회 실패: " << e.what() << std::endl;
    }
    return result;
}
