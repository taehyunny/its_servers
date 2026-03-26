
#pragma once
#include "StoreDTO.h"
#include <string>
#include <vector>
#include "json.hpp"

// =========================================================
// [기본 조립 블록] 검색 화면을 그리기 위한 단품 구조체들
// =========================================================

// 1. 인기 검색어 (카테고리명)
struct PopularKeyword
{
    int rank;
    std::string keyword;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(PopularKeyword, rank, keyword)
};

// 2. 최근 검색어 (팀원 요청 반영: PK는 historyId)
struct RecentSearch   // 클라이언트 -> 서버: "내 최근 검색어 삭제 요청입니다!" 요청 DTO
{
    int historyId;
    std::string keyword;
    std::string searchDate;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(RecentSearch, historyId, keyword, searchDate)
};

// =========================================================
// [프로토콜 매칭 DTO] 클라이언트 <-> 서버 통신용 구조체
// =========================================================

// [2108] 매장 검색 위젯 요청
struct ReqResearchWidgetDTO  // 클라이언트 -> 서버: "검색 위젯 초기 데이터 요청입니다!" 요청 DTO
{
    std::string userId; // 검색어 기록을 가져올 때 필요한 사용자 ID (핸들러의 req.userId에 대응)
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ReqResearchWidgetDTO, userId)
};

// [2109] 매장 검색 위젯 응답 (화면 초기화용)
struct ResResearchWidgetDTO  // 서버 -> 클라이언트: "검색 위젯 초기 데이터입니다!" 응답 DTO
{
    int status;                                  // 200: 성공, 400: 실패
    std::vector<PopularKeyword> popularKeywords; // 인기 카테고리 리스트 (예: "한식", "중식" 등)
    std::vector<RecentSearch> recentSearches;    // 내 최근 검색어 리스트 (예: "떡볶이", "피자" 등)
    std::string searchDate;                      // 검색어 데이터를 가져온 날짜 (예: "2024-06-15")
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ResResearchWidgetDTO, status, popularKeywords, recentSearches, searchDate)
};

// ---------------------------------------------------------

// [2110] 개별 검색어 삭제 요청 (X 버튼)
struct ReqResearchDeleteDTO // 클라이언트 -> 서버: "이 검색어 삭제 요청입니다!" 요청 DTO
{
    std::string userId; // 검색어 기록을 삭제할 때 필요한 사용자 ID (핸들러의 req.userId에 대응)
    int historyId;      // 팀원과 약속한 PK로 지우기!
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ReqResearchDeleteDTO, userId, historyId)
};

// [2111] 개별 검색어 삭제 응답
struct ResResearchDeleteDTO // 서버 -> 클라이언트: "이 검색어 삭제 결과입니다!" 응답 DTO
{
    int status; // 200: 성공, 400: 실패
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ResResearchDeleteDTO, status)
};

// ---------------------------------------------------------

// [2112] 검색어 추가 & 점수 획득 요청 (🚀 삼위일체 로직)
struct ReqResearchAddDTO   // 클라이언트 -> 서버: "검색어 추가 & 점수 획득 요청입니다!" 요청 DTO
{
    std::string userId;  // 검색어 기록을 추가할 때 필요한 사용자 ID (핸들러의 req.userId에 대응)
    std::string keyword; // "떡", "떡볶이", "신전떡볶이"
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ReqResearchAddDTO, userId, keyword)
};

// [2113] 검색어 추가 응답
struct ResResearchAddDTO   // 서버 -> 클라이언트: "검색어 추가 결과입니다!" 응답 DTO
{
    int status; // 200: 성공, 400: 실패
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ResResearchAddDTO, status)
};

// ---------------------------------------------------------

// [2114] 검색어 전체 삭제 요청
struct ReqResearchDelAllDTO
{
    std::string userId; // 검색어 기록을 전체 삭제할 때 필요한 사용자 ID (핸들러의 req.userId에 대응)
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ReqResearchDelAllDTO, userId)
};

// [2115] 검색어 전체 삭제 응답
struct ResResearchDelAllDTO   // 서버 -> 클라이언트: "검색어 전체 삭제 결과입니다!" 응답 DTO
{
    int status; // 200: 성공, 400: 실패
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ResResearchDelAllDTO, status)
};

// ---------------------------------------------------------

// [2116] 실시간 매장 검색 요청 ("떡" 쳤을 때)
struct ReqSearchStoreDTO  // 클라이언트 -> 서버: "이 검색어가 포함된 매장 다 주세요!" 요청 DTO
{
    std::string keyword; // 검색어 (핸들러의 req.keyword에 대응, "떡", "떡볶이", "신전떡볶이" 등)
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ReqSearchStoreDTO, keyword)
};

// [2117] 실시간 매장 검색 응답
struct ResSearchStoreDTO   // 서버 -> 클라이언트: "이 검색어가 포함된 매장 리스트입니다!" 응답 DTO
{
    int status;                          // 200: 성공, 400: 실패
    std::vector<TopStoreInfo> storeList; // 포함된 매장 리스트 쫙 뿌려주기
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ResSearchStoreDTO, status, storeList)
};