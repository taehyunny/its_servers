#pragma once
#include <string>
#include <vector>
#include "json.hpp"

// =========================================================
// [매장 상세 페이지용 DTO] 카멜/스네이크 완벽 대응 파서
// =========================================================
struct StoreDataDTO   //  매장 상세 정보 DTO (프론트엔드 요청 2002번에 대응)
{
    int storeId;     // 매장 ID (PK)
    std::string storeName;  // 매장 이름 (예: "맛있는 떡볶이")
    std::string storeAddress;  // 매장 주소 (예: "서울시 강남구 테헤란로 123")
    std::string operatingHours;  // 영업 시간 (예: "10:00 ~ 22:00")
    std::string deliveryFees;  // 배달비 정보 (예: "3,000원", "무료" 등)
    bool isOpen;  // 영업 중 여부 (true: 영업 중, false: 영업 종료)
    std::string imageUrl;  // 매장 이미지 URL (예: "https://example.com/images/store123.jpg")
    int minOrderAmount; // 최소 주문 금액 (예: 15000)
    double rating;   // 평점 (예: 4.5)
    int reviewCount; // 리뷰 수 (예: 120)
    std::string deliveryTimeRange;  // 배달 예상 시간 범위 (예: "30~40분")
    std::string pickupTime;   // 픽업 예상 시간 (예: "20분")
    std::string brandName; // 편의점 브랜드명 (예: "GS25", "CU", "7-Eleven")
    std::string category;  // 카테고리명 (예: "한식", "중식", "편의점" 등)
                          // 🚀 [추가] 프론트엔드 요청 데이터 3종 세트
    std::string phoneNumber;  // 전화번호 (예: "02-1234-5678")
    std::string representativeName; // 대표자명 (예: "홍길동")
    std::string businessNumber; // 사업자 등록번호 (예: "123-45-67890")

    // 🚀 서버 -> 클라이언트 (전송용)
    friend void to_json(nlohmann::json &j, const StoreDataDTO &dto)
    {
        j = nlohmann::json{
            {"storeId", dto.storeId},
            {"storeName", dto.storeName},
            {"storeAddress", dto.storeAddress},
            {"operatingHours", dto.operatingHours},
            {"deliveryFees", dto.deliveryFees},
            {"isOpen", dto.isOpen},
            {"imageUrl", dto.imageUrl},
            {"minOrderAmount", dto.minOrderAmount},
            {"rating", dto.rating},
            {"reviewCount", dto.reviewCount},
            {"deliveryTimeRange", dto.deliveryTimeRange},
            {"pickupTime", dto.pickupTime},
            {"brandName", dto.brandName},
            {"category", dto.category}, // 🚀 카테고리도 추가로 보내줍니다! (예: "한식", "중식", "편의점" 등)
            // 🚀 [추가] JSON 응답에 포함
            {"phoneNumber", dto.phoneNumber},
            {"representativeName", dto.representativeName},
            {"businessNumber", dto.businessNumber}};
    }

    // 🚀 클라이언트 -> 서버 (수신용 - 만능 방패)
    friend void from_json(const nlohmann::json &j, StoreDataDTO &dto)
    {
        dto.storeId = j.value("storeId", j.value("store_id", 0));
        dto.storeName = j.value("storeName", j.value("store_name", ""));
        dto.storeAddress = j.value("storeAddress", j.value("store_address", ""));
        dto.operatingHours = j.value("operatingHours", j.value("operating_hours", ""));
        dto.deliveryFees = j.value("deliveryFees", j.value("delivery_fees", ""));
        dto.isOpen = j.value("isOpen", j.value("is_open", false));
        dto.imageUrl = j.value("imageUrl", j.value("image_url", ""));
        dto.minOrderAmount = j.value("minOrderAmount", j.value("min_order_amount", 0));
        dto.rating = j.value("rating", 0.0);
        dto.reviewCount = j.value("reviewCount", j.value("review_count", 0));
        dto.deliveryTimeRange = j.value("deliveryTimeRange", j.value("delivery_time_range", ""));
        dto.pickupTime = j.value("pickupTime", j.value("pickup_time", ""));
        dto.brandName = j.value("brandName", j.value("brand_name", "정보 없음"));

        // 🚀 [추가] 역직렬화 방패
        dto.phoneNumber = j.value("phoneNumber", j.value("phone_number", "정보 없음"));
        dto.representativeName = j.value("representativeName", j.value("representative_name", "정보 없음"));
        dto.businessNumber = j.value("businessNumber", j.value("business_number", "정보 없음"));
    }
};

// ── 2. 단일 메뉴 정보 ──
struct MenuDataDTO  // 🚀 [구조 변경 핵심] 메뉴 상세 정보용 신규 DTO 
{
    int menuId;   // 메뉴 ID (PK)
    int storeId;  // 어떤 매장에 속한 메뉴인지 (FK)
    std::string menuName; // 메뉴 이름 (예: "황궁짜장면")
    int basePrice; // 기본 가격 (옵션 제외)
    bool isSoldOut;  // 품절 여부
    std::string description;  // 메뉴 설명 (예: "쫄깃한 면발과 진한 소스의 조화!")
    std::string imageUrl;   // 메뉴 이미지 URL (예: "https://example.com/images/jjajangmyeon.jpg")
    std::string menuCategory;  // 메뉴 카테고리 (예: "면류", "밥류", "사이드" 등)
    bool isPopular;  // 인기 메뉴 여부 (예: true면 "인기" 뱃지 표시)
    nlohmann::json optionGroups; // 🚀 옵션 그룹 추가! 
    std::string reviewContent; // 🚀 메뉴 리뷰 내용 (예: "이 메뉴 정말 맛있어요!")
    friend void to_json(nlohmann::json &j, const MenuDataDTO &dto)
    {
        j = nlohmann::json{
            {"menuId", dto.menuId},
            {"storeId", dto.storeId},
            {"menuName", dto.menuName},
            {"basePrice", dto.basePrice},
            {"isSoldOut", dto.isSoldOut},
            {"description", dto.description},
            {"imageUrl", dto.imageUrl},
            {"menuCategory", dto.menuCategory},
            {"isPopular", dto.isPopular},
            {"optionGroups", dto.optionGroups},
            {"reviewContent", dto.reviewContent}};
    }

    friend void from_json(const nlohmann::json &j, MenuDataDTO &dto)
    {
        dto.menuId = j.value("menuId", j.value("menu_id", 0));
        dto.storeId = j.value("storeId", j.value("store_id", 0));
        dto.menuName = j.value("menuName", j.value("menu_name", ""));
        dto.basePrice = j.value("basePrice", j.value("base_price", 0));
        dto.isSoldOut = j.value("isSoldOut", j.value("is_sold_out", false));
        dto.description = j.value("description", "");
        dto.imageUrl = j.value("imageUrl", j.value("image_url", ""));
        dto.menuCategory = j.value("menuCategory", j.value("menu_category", "기본 메뉴"));
        dto.isPopular = j.value("isPopular", j.value("is_popular", false));
        dto.optionGroups = j.value("optionGroups", j.value("option_groups", nlohmann::json::array()));
        dto.reviewContent = j.value("reviewContent", j.value("review_content", ""));
    }
};

// ── 3. 단일 리뷰 정보 ──
struct ReviewDTO    // 🚀 [구조 변경 핵심] 메뉴 리뷰용 신규 DTO
{
    int reviewId;   // 리뷰 ID (PK)
    std::string orderId;// 주문 ID (어떤 주문에 대한 리뷰인지)
    std::string userId;// 리뷰 작성자 ID
    int storeId;// 리뷰 대상 상점 ID

    // 🚀 [구조 변경 핵심] 메뉴 리뷰용 신규 데이터 2종
    int menuId;           // DB 조회 및 저장용 (어떤 메뉴인가?)
    std::string menuName; // 프론트엔드 표시용 (예: "황궁짜장면")

    int rating; //  별점 (1~5)
    std::string content;// 리뷰 내용
    std::string imageUrl;// 리뷰 이미지 URL (선택)
    std::string ownerReply; // 사장님 답글 (선택)
    std::string createdAt; // 리뷰 작성 시각 (예: "2024-08-15 14:30:00")

    // 🚀 서버 -> 클라이언트 (응답 시 JSON 조립)
    friend void to_json(nlohmann::json &j, const ReviewDTO &dto)
    {
        j = nlohmann::json{
            {"reviewId", dto.reviewId},
            {"orderId", dto.orderId},
            {"userId", dto.userId},
            {"storeId", dto.storeId},
            {"menuId", dto.menuId},     // 👈 추가
            {"menuName", dto.menuName}, // 👈 추가
            {"rating", dto.rating},
            {"content", dto.content},
            {"imageUrl", dto.imageUrl},
            {"ownerReply", dto.ownerReply},
            {"createdAt", dto.createdAt}};
    }

    // 🚀 클라이언트 -> 서버 (요청 시 JSON 파싱 - 뱀처럼 유연한 방패!)
    friend void from_json(const nlohmann::json &j, ReviewDTO &dto)
    {
        dto.reviewId = j.value("reviewId", j.value("review_id", 0));
        dto.orderId = j.value("orderId", j.value("order_id", ""));
        dto.userId = j.value("userId", j.value("user_id", ""));
        dto.storeId = j.value("storeId", j.value("store_id", 0));

        // 👈 카멜케이스(menuId)와 스네이크케이스(menu_id) 모두 대응!
        dto.menuId = j.value("menuId", j.value("menu_id", 0));
        dto.menuName = j.value("menuName", j.value("menu_name", ""));

        dto.rating = j.value("rating", 0);
        dto.content = j.value("content", "");
        dto.imageUrl = j.value("imageUrl", j.value("image_url", ""));
        dto.ownerReply = j.value("ownerReply", j.value("owner_reply", ""));
        dto.createdAt = j.value("createdAt", j.value("created_at", ""));
    }
};

// =========================================================
// [최종 응답] 2302번 프로토콜: 매장 상세 정보 응답 패키지
// =========================================================
struct ResStoreDetailDTO  // 서버 -> 클라이언트: "이 매장 상세 정보입니다!" 응답 DTO
{
    int status;   // 200: 성공, 400: 실패
    StoreDataDTO storeData;   // 매장 상세 정보
    std::vector<MenuDataDTO> menuList;  // 메뉴 리스트
    std::vector<ReviewDTO> reviewList;  // 리뷰 리스트
    std::string storeAddress;   // 🚀 [추가] 매장 주소도 별도로 담아줍니다 (storeData.storeAddress와 중복이지만 프론트엔드 요청 반영)

    friend void to_json(nlohmann::json &j, const ResStoreDetailDTO &dto)
    {
        j = nlohmann::json{
            {"status", dto.status},
            {"storeData", dto.storeData},
            {"menuList", dto.menuList},
            {"reviewList", dto.reviewList},
            {"storeAddress", dto.storeAddress}};
    }

    friend void from_json(const nlohmann::json &j, ResStoreDetailDTO &dto)
    {
        dto.status = j.value("status", 0);
        if (j.contains("storeData"))
            j.at("storeData").get_to(dto.storeData);
        if (j.contains("menuList"))
            j.at("menuList").get_to(dto.menuList);
        if (j.contains("reviewList"))
            j.at("reviewList").get_to(dto.reviewList);
        dto.storeAddress = j.value("storeAddress", j.value("store_address", ""));
    }
};

// =========================================================
// [요청] 2301번 프로토콜: 매장 클릭 시 요청
// =========================================================
struct ReqStoreDetailDTO // 클라이언트 -> 서버: "이 매장 상세 정보 요청입니다!" 요청 DTO
{
    int storeId; // 요청 시 매장 ID만 보내면 됩니다!

    friend void to_json(nlohmann::json &j, const ReqStoreDetailDTO &dto)
    {
        j = nlohmann::json{{"storeId", dto.storeId}};
    }

    // 🚀 서버에서 파싱할 때 필요함!
    friend void from_json(const nlohmann::json &j, ReqStoreDetailDTO &dto)
    {
        dto.storeId = j.value("storeId", j.value("store_id", 0));
    }
};