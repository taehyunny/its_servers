#pragma once
#include <string>
#include <vector>
#include "json.hpp"

struct StoreDTO
{                                // 상점 정보를 나타내는 DTO
    int storeId;                 // 상점 ID
    std::string storeName;       // 상점 이름
    std::string category;        // 상점 카테고리 (예: "한식", "중식", "패스트푸드" 등)
    int status;                  // 영업 상태 (0: 영업 중, 1: 휴업, 2: 폐업 등)
    nlohmann::json deliveryFees; // 배달팁 JSON

    // [추가] 실시간 조리시간
    int cookTime;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(StoreDTO, storeId, storeName, category, status, deliveryFees)
};

struct StoreListResDTO
{                                 // 상점 목록 응답 DTO
    int status;                   // 0: 성공, 1: 실패
    std::string message;          // 응답 메시지
    std::vector<StoreDTO> stores; // 상점 목록
    std::vector<int> cookTimes;   // 상점별 조리 시간 목록
    int cookTime;
    // [추가] cookTime 추가
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(StoreListResDTO, status, message, stores, cookTime)
};

struct MenuDTO
{                               //  메뉴 정보를 나타내는 DTO
    int menuId;                 // 메뉴 ID
    std::string menuName;       // 메뉴 이름
    int basePrice;              // 기본 가격 (옵션 제외)
    int isSoldOut;              // 품절 여부 (0: 판매 중, 1: 품절)
    nlohmann::json menuOptions; // Qt 동적 UI 렌더링용 JSON

    // [추가] 메뉴 상세 설명
    std::string description;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(MenuDTO, menuId, menuName, basePrice, isSoldOut, menuOptions, description)
};

struct MenuListResDTO
{                               // 메뉴 목록 응답 DTO
    int status;                 // 0: 성공, 1: 실패
    std::string message;        // 응답 메시지
    int storeId;                // 메뉴 목록이 속한 상점 ID (클라이언트가 어떤 상점의 메뉴인지 알 수 있도록)
    std::vector<MenuDTO> menus; // 메뉴 목록

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(MenuListResDTO, status, message, storeId, menus)
};

// ************************************************************
// [추가] 사장님 파트 전용 요청(REQ) 객체 추가
// ************************************************************

// 조리 시간 및 영업 상태 변경 요청
struct StoreStatusUpdateReqDTO
{
    int storeId;
    int updateType;  // 1: 영업상태 변경, 2: 조리시간 변경
    int statusValue; // 바꿀 상태값(0: 휴업, 1: 영업중) 또는 조리시간(분 단위)

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(StoreStatusUpdateReqDTO, storeId, updateType, statusValue)
};

// 메뉴 등록/수정/삭제/품절 관리 요청
struct MenuUpdateReqDTO
{
    int storeId;
    int actionType;   // 1: 신규등록, 2: 정보수정, 3: 삭제, 4: 품절상태변경
    MenuDTO menuData; // 변경하거나 새로 등록할 메뉴의 전체 정보

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(MenuUpdateReqDTO, storeId, actionType, menuData)
};