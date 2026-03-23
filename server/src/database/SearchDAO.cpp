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
        // 점수 내림차순으로 상위 N개 가져오기
        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
            "SELECT name FROM CATEGORIES ORDER BY popularity_score DESC LIMIT ?"));
        pstmt->setInt(1, limit);
        std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());

        int currentRank = 1;
        while (rs->next())
        {
            PopularKeyword pk;
            pk.rank = currentRank++;
            pk.keyword = rs->getString("name").c_str(); // 카테고리명을 키워드로 사용
            result.push_back(pk);
        }
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "[SearchDAO] 인기 카테고리 조회 실패: " << e.what() << std::endl;
    }
    return result;
}

std::vector<RecentSearch> SearchDAO::getRecentSearches(const std::string &userId)
{
    std::vector<RecentSearch> result;
    try
    {
        auto conn = DBManager::getInstance().getConnection();
        // 삭제되지 않은(is_visible=1) 내 검색어를 최신순으로 10개만!
        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
            "SELECT history_id, keyword FROM SEARCH_HISTORY WHERE user_id = ? AND is_visible = 1 ORDER BY search_date DESC LIMIT 10"));
        pstmt->setString(1, userId);
        std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());

        while (rs->next())
        {
            RecentSearch search;
            search.historyId = rs->getInt("history_id");
            search.keyword = rs->getString("keyword").c_str();
            result.push_back(search);
        }
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "[SearchDAO] 최근 검색어 조회 실패: " << e.what() << std::endl;
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
        pstmt->executeUpdate(); // 데이터를 변경할 때는 executeUpdate!
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

        // =========================================================
        // 1순위: 완벽한 매장명 검색인지 확인 (STORES 테이블 업데이트)
        // =========================================================
        std::unique_ptr<sql::PreparedStatement> pstmtStore(conn->prepareStatement(
            "UPDATE STORES SET popularity_score = popularity_score + 1 WHERE name = ?"));
        pstmtStore->setString(1, keyword);
        int rowsUpdated = pstmtStore->executeUpdate(); // 업데이트된 행의 개수 반환

        // 만약 매장명이 정확히 일치해서 점수가 올랐다면? 여기서 로직 끝!
        if (rowsUpdated > 0)
        {
            std::cout << "[SearchDAO] 매장명 일치! 매장 점수 +1 (" << keyword << ")" << std::endl;
            return true;
        }

        // =========================================================
        // 2 & 3순위: 매장명이 아니라면, 메뉴(정확 or 부분)를 찾아 카테고리 점수 올리기
        // =========================================================
        // 서브쿼리 설명: 메뉴 이름에 키워드가 포함된 걸 찾고 -> 그 메뉴의 가게를 찾고 -> 그 가게의 카테고리 점수를 +1
        std::unique_ptr<sql::PreparedStatement> pstmtCategory(conn->prepareStatement(
            "UPDATE CATEGORIES SET popularity_score = popularity_score + 1 "
            "WHERE name = ("
            "    SELECT S.category "
            "    FROM MENUS M "
            "    JOIN STORES S ON M.store_id = S.store_id "
            "    WHERE M.name LIKE ? LIMIT 1"
            ")"));

        std::string searchPattern = "%" + keyword + "%"; // "돈" -> "%돈%"
        pstmtCategory->setString(1, searchPattern);
        int catUpdated = pstmtCategory->executeUpdate();

        if (catUpdated > 0)
        {
            std::cout << "[SearchDAO] 메뉴명(부분) 일치! 카테고리 점수 +1 (" << keyword << ")" << std::endl;
        }
        else
        {
            // 매장도 없고 메뉴에도 없으면 (ex: "ㅋㅋㅋ") 그냥 넘어감
            std::cout << "[SearchDAO] 검색어에 해당하는 매장/메뉴가 없습니다. (" << keyword << ")" << std::endl;
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
        // 매장 이름에 키워드가 포함된 것 찾기
        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
            "SELECT store_id, name, category, rating, review_count, min_order_price, delivery_time, delivery_fee, icon_path FROM STORES WHERE name LIKE ?"));

        // LIKE 검색을 위해 앞뒤에 % 붙여주기 (예: "%떡%")
        std::string searchPattern = "%" + keyword + "%";
        pstmt->setString(1, searchPattern);

        std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());

        while (rs->next())
        {
            TopStoreInfo store;
            store.storeId = rs->getInt("store_id");
            store.storeName = rs->getString("name").c_str();
            store.category = rs->getString("category").c_str();
            store.rating = rs->getDouble("rating");
            store.reviewCount = rs->getInt("review_count");
            store.minOrderPrice = rs->getInt("min_order_price");
            store.deliveryTime = rs->getString("delivery_time").c_str();
            store.deliveryFee = rs->getInt("delivery_fee");
            store.iconPath = rs->getString("icon_path").c_str();
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
// 4. 최근 검색어 삭제 로직 (논리적 삭제)
// =========================================================

bool SearchDAO::deleteRecentSearch(const std::string &userId, int historyId)
{
    try
    {
        auto conn = DBManager::getInstance().getConnection();

        // 유저 ID와 고유번호(PK)가 정확히 일치하는 데이터만 화면에서 숨김(0) 처리!
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
        // 해당 유저의 모든 검색어를 안 보이게 처리
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