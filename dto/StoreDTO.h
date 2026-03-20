#pragma once
#include <string>
#include <vector>
#include "json.hpp"

// 1. 순서 교정: MenuDTO가 먼저 와야 다른 곳에서 쓸 수 있습니다.
struct MenuDTO
{
    int menuId;
    std::string menuName;
    int basePrice;

    // 🚀 int 대신 bool로 통일! (클라이언트에서도 true/false로 받는 게 편합니다)
    bool isSoldOut;

    nlohmann::json menuOptions;
    std::string description;
    std::string imageUrl;
    std::string menuCategory;
    bool isPopular;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(MenuDTO, menuId, menuName, basePrice, isSoldOut, menuOptions, description, imageUrl, menuCategory, isPopular)
};

// 2. StoreDTO (개별 가게 정보)
struct StoreDTO
{
    int storeId;
    std::string storeName;
    std::string category;
    int status;
    nlohmann::json deliveryFees;
    int cookTime;
    std::string imageUrl;
    int minOrderAmount;
    double rating;
    int reviewCount;
    std::string deliveryTimeRange;

    // 🚀 [추가된 컬럼들]
    std::string storeAddress; // 매장 주소
    std::string openTime;     // 오픈 시간 (예: "09:00")
    std::string closeTime;    // 마감 시간 (예: "22:00")

    MenuDTO popularMenu;

    // ⚠️ 매크로 마지막 부분에 새 변수 3개를 꼭 추가해야 합니다!
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(StoreDTO,
                                   storeId, storeName, category, status, deliveryFees, cookTime,
                                   imageUrl, minOrderAmount, rating, reviewCount, deliveryTimeRange,
                                   storeAddress, openTime, closeTime, popularMenu)
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