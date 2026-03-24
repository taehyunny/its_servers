#include <string>
#include "json.hpp"

// =========================================================
// [매장 상세 페이지용 DTO] 프론트엔드 구조체와 1:1 매칭
// =========================================================

// 1. 상점 상세 정보
struct StoreDataDTO
{
    int store_id;
    std::string store_name;
    std::string store_address;
    std::string operating_hours;
    std::string delivery_fees; // 클라이언트가 문자열로 받길 원함 (예: "3000원")
    bool is_open;
    std::string image_url;
    int min_order_amount;
    double rating;
    int review_count;
    std::string delivery_time_range; // 예: "30~40분"

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(StoreDataDTO,
                                   store_id, store_name, store_address, operating_hours, delivery_fees,
                                   is_open, image_url, min_order_amount, rating, review_count, delivery_time_range)
};

// 2. 단일 메뉴 정보
struct MenuDataDTO
{
    int menu_id;
    int store_id;
    std::string menu_name;
    int base_price;
    bool is_sold_out;
    std::string description;
    std::string image_url;
    std::string menu_category; // "메인 메뉴", "사이드" 등을 구분할 카테고리
    bool is_popular;           // 인기 메뉴 여부 (별지정 등)

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(MenuDataDTO,
                                   menu_id, store_id, menu_name, base_price, is_sold_out,
                                   description, image_url, menu_category, is_popular)
};

// 3. 단일 리뷰 정보
struct ReviewDataDTO
{
    int review_id;
    int store_id;
    std::string user_id;
    std::string order_id;
    int rating;
    std::string content;
    std::string created_at;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ReviewDataDTO,
                                   review_id, store_id, user_id, order_id, rating, content, created_at)
};

// =========================================================
// 🚀 [최종 응답] 2302번 프로토콜: 매장 상세 정보 응답 패키지
// =========================================================
struct ResStoreDetailDTO
{
    int status;                            // 200: 성공
    StoreDataDTO storeData;                // 매장 기본 정보 (1개)
    std::vector<MenuDataDTO> menuList;     // 메뉴 리스트 (여러 개)
    std::vector<ReviewDataDTO> reviewList; // 리뷰 리스트 (여러 개)

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ResStoreDetailDTO, status, storeData, menuList, reviewList)
};

// [요청] 2301번 프로토콜: 매장 클릭 시 요청
struct ReqStoreDetailDTO
{
    int store_id; // 어떤 매장을 클릭했는지 서버에 알려줌

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ReqStoreDetailDTO, store_id)
};