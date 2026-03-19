#pragma once
#include <string>
#include <vector>
#include "json.hpp"

// 1. 순서 교정: MenuDTO가 먼저 와야 다른 곳에서 쓸 수 있습니다.
struct MenuDTO // 개별 메뉴 정보
{
    int menuId;                 // 메뉴 고유 ID
    std::string menuName;       // 메뉴 이름
    int basePrice;              // 기본 가격 (옵션 제외)
    int isSoldOut;              // 품절 여부 (0: 판매 중, 1: 품절
    nlohmann::json menuOptions; // 옵션 정보 (JSON 형태로 유연하게 저장)
    std::string description;    // 메뉴 설명

    std::string imageUrl;     // 메뉴 이미지 URL (옵션)
    std::string menuCategory; // 메뉴 카테고리 (예: "사이드", "음료", "디저트")
    bool isPopular;           // 인기 메뉴 여부 (클라이언트 UI용)

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(MenuDTO, menuId, menuName, basePrice, isSoldOut, menuOptions, description, imageUrl, menuCategory, isPopular)
};

// 2. StoreDTO (개별 가게 정보)
struct StoreDTO
{
    int storeId;                 // 가게 고유 ID
    std::string storeName;       // 가게 이름
    std::string category;        // 가게 카테고리 (예: "한식", "중식", "패스트푸드")
    int status;                  // 가게 상태 (0: 영업 중, 1: 휴업, 2: 폐업)
    nlohmann::json deliveryFees; // 배달비 정보 (JSON 형태로 유연하게 저장)
    int cookTime;                // 예상 조리 시간 (분 단위)

    std::string imageUrl;          // 가게 이미지 URL (옵션)
    int minOrderAmount;            // 최소 주문 금액 (원 단위)
    double rating;                 // 가게 평점
    int reviewCount;               // 리뷰 수
    std::string deliveryTimeRange; // 배달 예상 시간 범위

    // [옵션] 목록에서 보여줄 대표 메뉴 1개 (클라이언트 UI용)
    MenuDTO popularMenu;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(StoreDTO, storeId, storeName, category, status, deliveryFees, cookTime, imageUrl, minOrderAmount, rating, reviewCount, deliveryTimeRange, popularMenu)
};

// 3. StoreListResDTO (가게 목록 전송용 껍데기)
// 군더더기 없이 리스트 자체만 전송합니다.
struct StoreListResDTO
{
    int status;                   // 0: 성공, 1: 실패
    std::string message;          // 상태 메시지 (예: "성공", "데이터 없음", "서버 오류")
    std::vector<StoreDTO> stores; // 모든 데이터는 이 안에 다 들어있습니다.

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(StoreListResDTO, status, message, stores)
};

// 4. MenuListResDTO (특정 가게의 상세 메뉴 전송용)
struct MenuListResDTO
{
    int status;                 // 0: 성공, 1: 실패
    std::string message;        // 상태 메시지 (예: "성공", "데이터 없음", "서버 오류")
    int storeId;                // 가게 ID
    std::vector<MenuDTO> menus; // 메뉴 목록

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(MenuListResDTO, status, message, storeId, menus)
};

// 5. 사장님 전용 REQ DTO (수정 없음, 완벽함)
struct StoreStatusUpdateReqDTO
{
    int storeId;     // 가게 ID
    int updateType;  // 업데이트 유형 (0: 영업 시작, 1: 휴업, 2: 폐업)
    int statusValue; // 업데이트할 상태 값 (0: 영업 중, 1: 휴업, 2: 폐업)
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(StoreStatusUpdateReqDTO, storeId, updateType, statusValue)
};

struct MenuUpdateReqDTO
{
    int storeId;      // 가게 ID
    int actionType;   // 작업 유형 (0: 메뉴 추가, 1: 메뉴 수정, 2: 메뉴 삭제)
    MenuDTO menuData; // 메뉴 데이터 (추가/수정 시 사용, 삭제 시 menuId만 필요)
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(MenuUpdateReqDTO, storeId, actionType, menuData)
};