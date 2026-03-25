#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mariadb/conncpp.hpp> // 🔓 주석 해제!
#include "AllDTOs.h"           // 🔓 주석 해제! (구조체들 포함)

class SearchDAO
{
private:
    // DB 커넥션을 직접 들고 있기보다는, 필요할 때마다 DBManager에서 빌려오는 방식이 더 안전합니다.
    // 하지만 클래스 멤버로 관리하고 싶다면 아래처럼 선언해두셔도 좋습니다.
    // std::shared_ptr<sql::Connection> conn;

public:
    SearchDAO() = default;
    ~SearchDAO() = default;

    // 1. 검색 위젯 초기화 (인기 카테고리 & 최근 검색어)
    std::vector<PopularKeyword> getPopularCategories(int limit = 10);
    std::vector<RecentSearch> getRecentSearches(const std::string &userId);

    // 2. 삼위일체 로직: 기록 저장 & 점수 반영
    bool insertSearchHistory(const std::string &userId, const std::string &keyword);
    bool applySearchScore(const std::string &keyword);

    // 3. 실시간 매장 검색 (LIKE %키워드%)
    std::vector<TopStoreInfo> searchStoresByKeyword(const std::string &keyword);

    // 4. 삭제 로직
    bool deleteRecentSearch(const std::string &userId, int historyId);
    bool deleteAllRecentSearches(const std::string &userId);

};