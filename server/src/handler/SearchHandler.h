#pragma once

#include <memory>
#include <string>
#include "AllDTOs.h"
#include "SearchDAO.h"
// #include "ClientSession.h" // 태현님 프로젝트의 세션 관리 클래스 헤더를 넣어주세요!

class ClientSession;

class SearchHandler
{
private:
    SearchDAO searchDao; // DB 작업을 전담할 DAO 일꾼

public:
    SearchHandler() = default;
    ~SearchHandler() = default;

    // 1. 위젯 초기화 요청 (2108 -> 2109)
    static void handleSearchWidgetReq(std::shared_ptr<ClientSession> session, const std::string &jsonBody);

    // 2. 검색 기록 추가 & 점수 반영 요청 (2112 -> 2113) - 🚀 삼위일체 핵심!
    static void handleSearchAddReq(std::shared_ptr<ClientSession> session, const std::string &jsonBody);

    // 3. 개별 검색어 삭제 요청 (2110 -> 2111)
    static void handleSearchDeleteReq(std::shared_ptr<ClientSession> session, const std::string &jsonBody);

    // 4. 전체 검색어 삭제 요청 (2114 -> 2115)
    static void handleSearchDelAllReq(std::shared_ptr<ClientSession> session, const std::string &jsonBody);

    // 5. 실시간 매장 검색 요청 (2116 -> 2117)
    static void handleSearchStoreReq(std::shared_ptr<ClientSession> session, const std::string &jsonBody);
    
};