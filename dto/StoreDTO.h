#pragma once
#include <string>
#include <vector>
#include "json.hpp" // 경로 확인 필수!

using json = nlohmann::json;

// 0. CategoryItem 정의 (누락되었던 부분 추가)

// 1. 순서 교정: 하위 항목부터 정의
struct OptionItem
{
    int optionId;
    std::string optionName;
    int additionalPrice;
    int displayOrder;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(OptionItem, optionId, optionName, additionalPrice, displayOrder)
};

// 2. 옵션 카테고리 (큰 틀)
struct OptionGroup
{
    int groupId;
    std::string groupName;
    bool isRequired;
    int maxCount;
    int displayOrder;
    std::vector<OptionItem> options;

    // 🚀 [해결책] to_json 추가 (서버 -> 클라이언트로 보낼 때 필요)
    friend void to_json(nlohmann::json &j, const OptionGroup &dto)
    {
        j = nlohmann::json{
            {"groupId", dto.groupId},
            {"groupName", dto.groupName},
            {"isRequired", dto.isRequired},
            {"maxCount", dto.maxCount},
            {"displayOrder", dto.displayOrder},
            {"options", dto.options}};
    }

    // 🚀 from_json 커스텀 구현 (클라이언트 -> 서버로 받을 때 필요)
    friend void from_json(const nlohmann::json &j, OptionGroup &dto)
    {
        j.at("groupId").get_to(dto.groupId);
        j.at("groupName").get_to(dto.groupName);
        dto.isRequired = j.value("isRequired", false);
        dto.maxCount = j.value("maxCount", 1);
        dto.displayOrder = j.value("displayOrder", 99);
        j.at("options").get_to(dto.options);
    }
};

// 3. 메뉴 DTO
struct MenuDTO
{
    int menuId;
    std::string menuName;
    int basePrice;
    bool isSoldOut;
    std::string description;
    std::string imageUrl;
    std::string menuCategory;
    bool isPopular;
    std::vector<OptionGroup> optionGroups;
    // 🚀 1. to_json: 서버 -> 클라이언트로 보낼 때 (DB 데이터를 JSON으로 변환)
    friend void to_json(nlohmann::json &j, const MenuDTO &dto)
    {
        j = nlohmann::json{
            {"menuId", dto.menuId},
            {"menuName", dto.menuName},
            {"basePrice", dto.basePrice},
            {"isSoldOut", dto.isSoldOut},
            {"isPopular", dto.isPopular},
            {"description", dto.description},
            {"imageUrl", dto.imageUrl},
            {"menuCategory", dto.menuCategory},
            // 🚀 핵심: 하위 배열인 OptionGroup도 연쇄적으로 자동 직렬화됩니다!
            {"optionGroups", dto.optionGroups}};
    }

    // 🚀 2. from_json: 클라이언트 -> 서버로 받을 때 (JSON을 C++ 구조체로 변환)
    friend void from_json(const nlohmann::json &j, MenuDTO &dto)
    {
        // 필수로 들어와야 하는 값들은 예외를 던지도록 at() 사용 (옵션)
        // 하지만 서버가 죽는 걸 막으려면 가급적 value()를 쓰는 것이 좋습니다.
        dto.menuId = j.value("menuId", 0);
        dto.menuName = j.value("menuName", "");
        dto.basePrice = j.value("basePrice", 0);

        // 프론트에서 값이 안 들어오면 안전하게 기본값 세팅
        dto.isSoldOut = j.value("isSoldOut", false);
        dto.isPopular = j.value("isPopular", false);
        dto.description = j.value("description", "");
        dto.imageUrl = j.value("imageUrl", "");
        dto.menuCategory = j.value("menuCategory", "기본 메뉴");

        // 🚀 방어 로직: optionGroups 배열이 통째로 빠져있어도 서버가 터지지 않게 방어!
        if (j.contains("optionGroups") && j.at("optionGroups").is_array())
        {
            j.at("optionGroups").get_to(dto.optionGroups);
        }
    }
};

struct MenuListReqDTO // 클라이언트 -> 서버: "이 가게 메뉴 다 주세요!" 요청 DTO
{
    int storeId;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(MenuListReqDTO, storeId)
};

// 🚀 응답 (RES_MENU_LIST = 2011)
// 서버가 "자, 1번 매장의 턔현님표 MenuDTO 리스트야!" 라고 응답할 때 씁니다.
struct MenuListResDTO
{
    int status;          // 0: 성공, 1: 실패
    std::string message; // "조회 성공" 등
    int storeId;         // 👈 어떤 가게의 메뉴인지 명시 (중요!)
    std::vector<MenuDTO> menus;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(MenuListResDTO, status, message, storeId, menus)
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

// 4. MenuListResDTO (특정 가게의 상세 메뉴 전송용)

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

struct TopStoreInfo
{
    int storeId;              // 매장 ID (검색 결과에서 상세 페이지로 넘어갈 때 필요)
    std::string storeName;    // 매장 이름
    std::string category;     // 매장 카테고리
    double rating;            // 평점
    int reviewCount;          // 리뷰 수
    int minOrderPrice;        // 최소 주문 금액
    std::string deliveryTime; // 배달 시간 (예: "30~40분")
    int deliveryFee;          // 배달비
    std::string iconPath;
    std::string deliveryTimeRange; // 배달 시간 범위 (예: "30~40분")
    int minOrderAmount;            // 최소 주문 금액
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(TopStoreInfo, storeId, storeName, category, rating, reviewCount, minOrderPrice, deliveryTime, deliveryFee, iconPath, deliveryTimeRange, minOrderAmount)
};
// 🚀 메인 화면용 '통합' 응답 패킷!
struct MainHomeResDTO
{
    int status;
    std::vector<CategoryItem> categories; // 위에는 카테고리
    std::vector<TopStoreInfo> topStores;  // 아래는 1등 매장들!

    // 이 한 줄로 JSON 변환 끝!
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(MainHomeResDTO, status, categories, topStores)
};

// 🚀 클라이언트 -> 서버: "이 카테고리 매장 다 주세요!" (예: REQ_STORES_BY_CATEGORY)
struct StoreListReqDTO
{
    int categoryId;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(StoreListReqDTO, categoryId)
};

// 🚀 서버 -> 클라이언트: "여기 주문하신 매장 리스트입니다!" (예: RES_STORES_BY_CATEGORY)
// 매장 정보는 아까 만든 TopStoreInfo를 이름만 StoreInfo로 바꿔서 재활용하면 개꿀입니다! ㅋㅋㅋ
struct StoreListResDTO
{
    int status;
    std::string message;              // 메시지 필드가 필요하다면 유지
    std::vector<TopStoreInfo> stores; // TopStoreInfo 바구니!

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(StoreListResDTO, status, message, stores)
};

// 사장님 사업자 번호 중복 체크용 DTO
struct BizNumCheckReqDTO
{
    std::string businessNum;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(BizNumCheckReqDTO, businessNum)
};

struct BizNumCheckResDTO
{
    int status;
    bool isAvailable;
    std::string message;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(BizNumCheckResDTO, status, isAvailable, message)
};

// 1. 개별 옵션 항목 (작은 틀)
