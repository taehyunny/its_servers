#pragma once
#include <string>
#include <vector>
#include "json.hpp"

// =========================================================
// [매장 상세 페이지용 DTO] 카멜/스네이크 완벽 대응 파서
// =========================================================

// ── 1. 상점 상세 정보 ──
// ── 1. 상점 상세 정보 ──
struct StoreDataDTO
{
    int storeId;
    std::string storeName;
    std::string storeAddress;
    std::string operatingHours;
    std::string deliveryFees;
    bool isOpen;
    std::string imageUrl;
    int minOrderAmount;
    double rating;
    int reviewCount;
    std::string deliveryTimeRange;

    // 🚀 [추가] 프론트엔드 요청 데이터 3종 세트
    std::string phoneNumber;
    std::string representativeName;
    std::string businessNumber;

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

        // 🚀 [추가] 역직렬화 방패
        dto.phoneNumber = j.value("phoneNumber", j.value("phone_number", "정보 없음"));
        dto.representativeName = j.value("representativeName", j.value("representative_name", "정보 없음"));
        dto.businessNumber = j.value("businessNumber", j.value("business_number", "정보 없음"));
    }
};

// ── 2. 단일 메뉴 정보 ──
struct MenuDataDTO
{
    int menuId;
    int storeId;
    std::string menuName;
    int basePrice;
    bool isSoldOut;
    std::string description;
    std::string imageUrl;
    std::string menuCategory;
    bool isPopular;
    nlohmann::json optionGroups; // 🚀 옵션 그룹 추가!

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
            {"optionGroups", dto.optionGroups}};
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
    }
};

// ── 3. 단일 리뷰 정보 ──
struct ReviewDTO
{
    int reviewId;
    std::string orderId;
    std::string userId;
    int storeId;
    int rating;
    std::string content;
    std::string imageUrl;
    std::string ownerReply;
    std::string createdAt;

    friend void to_json(nlohmann::json &j, const ReviewDTO &dto)
    {
        j = nlohmann::json{
            {"reviewId", dto.reviewId},
            {"orderId", dto.orderId},
            {"userId", dto.userId},
            {"storeId", dto.storeId},
            {"rating", dto.rating},
            {"content", dto.content},
            {"imageUrl", dto.imageUrl},
            {"ownerReply", dto.ownerReply},
            {"createdAt", dto.createdAt}};
    }

    friend void from_json(const nlohmann::json &j, ReviewDTO &dto)
    {
        dto.reviewId = j.value("reviewId", j.value("review_id", 0));
        dto.orderId = j.value("orderId", j.value("order_id", ""));
        dto.userId = j.value("userId", j.value("user_id", ""));
        dto.storeId = j.value("storeId", j.value("store_id", 0));
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
struct ResStoreDetailDTO
{
    int status;
    StoreDataDTO storeData;
    std::vector<MenuDataDTO> menuList;
    std::vector<ReviewDTO> reviewList;

    friend void to_json(nlohmann::json &j, const ResStoreDetailDTO &dto)
    {
        j = nlohmann::json{
            {"status", dto.status},
            {"storeData", dto.storeData},
            {"menuList", dto.menuList},
            {"reviewList", dto.reviewList}};
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
    }
};

// =========================================================
// [요청] 2301번 프로토콜: 매장 클릭 시 요청
// =========================================================
struct ReqStoreDetailDTO
{
    int storeId;

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