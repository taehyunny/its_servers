#include "SearchHandler.h"
#include "ClientSession.h"
#include "AllDTOs.h"
#include "TimeUtil.h"

#include "Global_protocol.h"
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// =========================================================
// 1. 검색 위젯 데이터 요청 (2108) -> 응답 (2109)
// =========================================================
void SearchHandler::handleSearchWidgetReq(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {

        std::cout << "\n📦 [SearchHandler 디버그] 수신된 원본 JSON: [" << jsonBody << "]" << std::endl;
        SearchDAO searchDao;
        json reqJson = json::parse(jsonBody);
        ReqResearchWidgetDTO reqDto = reqJson.get<ReqResearchWidgetDTO>();

        ResResearchWidgetDTO resDto;
        resDto.status = 200;

        // 🚀 여기서 TimeUtil을 사용해 오늘 날짜를 세팅합니다!
        resDto.searchDate = TimeUtil::getCurrentDate();

        resDto.popularKeywords = searchDao.getPopularCategories(10);
        resDto.recentSearches = searchDao.getRecentSearches(reqDto.userId);

        session->sendPacket(static_cast<int>(CmdID::RES_RESEARCH_WIDGET), json(resDto));
    }
    catch (const std::exception &e)
    {
        std::cerr << "[SearchHandler] 위젯 처리 에러: " << e.what() << std::endl;
    }
}

// =========================================================
// 2. 검색어 추가 & 지능형 점수 반영 (2112) -> 응답 (2113)
// =========================================================
void SearchHandler::handleSearchAddReq(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        SearchDAO searchDao;
        json reqJson = json::parse(jsonBody);
        ReqResearchAddDTO reqDto = reqJson.get<ReqResearchAddDTO>();

        // 1. 히스토리에 무조건 저장 (is_visible = 1)
        searchDao.insertSearchHistory(reqDto.userId, reqDto.keyword);

        // 2. 태현님의 "삼위일체 알고리즘" 발동! (알아서 매장인지 메뉴인지 판별해 점수 UP)
        searchDao.applySearchScore(reqDto.keyword);

        // 3. 성공 응답 (프로토콜 ID: 2113)
        ResResearchAddDTO resDto{200};
        session->sendPacket(static_cast<int>(CmdID::RES_RESEARCH_ADD), json(resDto));

        std::cout << "[SearchHandler] 검색 기록 추가 및 점수 반영 완료 (" << reqDto.keyword << ")" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "[SearchHandler] 검색 추가 요청 처리 중 에러: " << e.what() << std::endl;
    }
}

// =========================================================
// 3. 개별 검색어 삭제 (2110) -> 응답 (2111)
// =========================================================
void SearchHandler::handleSearchDeleteReq(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        SearchDAO searchDao;
        json reqJson = json::parse(jsonBody);
        ReqResearchDeleteDTO reqDto = reqJson.get<ReqResearchDeleteDTO>();

        // DAO에서 keyword를 기반으로 삭제하도록 호출 (DAO 수정 필요 시 참고)
        searchDao.deleteRecentSearch(reqDto.userId, reqDto.historyId);

        ResResearchDeleteDTO resDto{200};
        session->sendPacket(static_cast<int>(CmdID::RES_RESEARCH_DELETE), json(resDto));
    }
    catch (const std::exception &e)
    {
        std::cerr << "[SearchHandler] 검색어 개별 삭제 에러: " << e.what() << std::endl;
    }
}

// =========================================================
// 4. 전체 검색어 삭제 (2114) -> 응답 (2115)
// =========================================================
void SearchHandler::handleSearchDelAllReq(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        SearchDAO searchDao;
        json reqJson = json::parse(jsonBody);
        ReqResearchDelAllDTO reqDto = reqJson.get<ReqResearchDelAllDTO>();

        searchDao.deleteAllRecentSearches(reqDto.userId);

        ResResearchDelAllDTO resDto{200};
        session->sendPacket(static_cast<int>(CmdID::RES_RESEARCH_DEL_ALL), json(resDto));
    }
    catch (const std::exception &e)
    {
        std::cerr << "[SearchHandler] 검색어 전체 삭제 에러: " << e.what() << std::endl;
    }
}

// =========================================================
// 5. 실시간 매장 검색 (2116) -> 응답 (2117)
// =========================================================
void SearchHandler::handleSearchStoreReq(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        SearchDAO searchDao;
        json reqJson = json::parse(jsonBody);
        ReqSearchStoreDTO reqDto = reqJson.get<ReqSearchStoreDTO>();

        ResSearchStoreDTO resDto;
        resDto.storeList = searchDao.searchStoresByKeyword(reqDto.keyword);

        // 검색 결과가 있으면 200, 없으면 404 세팅
        resDto.status = resDto.storeList.empty() ? 404 : 200;

        session->sendPacket(static_cast<int>(CmdID::RES_SEARCH_STORE), json(resDto));

        std::cout << "[SearchHandler] 실시간 매장 검색 완료 (" << reqDto.keyword << " -> " << resDto.storeList.size() << "건 발견)" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "[SearchHandler] 매장 검색 에러: " << e.what() << std::endl;
    }
}

