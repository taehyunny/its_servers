#pragma once
#include <string>
#include <vector>
#include "json.hpp"

struct BaseResponseDTO
{
    int status;
    std::string message;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(BaseResponseDTO, status, message)
};

struct CategoryItem
{
    int id = 0;
    std::string name = "";
    std::string iconPath = "";
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(CategoryItem, id, name, iconPath)
};

struct BrandCategoryItem
{
    std::string brandName = "";
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(BrandCategoryItem, brandName)
};

// 🚀 [해결의 핵심] 통합된 완전체 TopStoreInfo (모든 변수 0, "" 로 100% 초기화!)
struct TopStoreInfo
{
    int storeId = 0;
    std::string storeName = "";
    std::string category = "";
    std::string brandName = "";
    int minOrderPrice = 0;
    std::string deliveryTime = "";
    int deliveryFee = 0;
    std::string iconPath = "";
    std::string deliveryTimeRange = "";
    std::string pickupTime = "";
    double rating = 0.0;
    int reviewCount = 0;
    int minOrderAmount = 0;
    std::string grade = "";

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(TopStoreInfo, 
        storeId, storeName, category, brandName, minOrderPrice, 
        deliveryTime, deliveryFee, iconPath, deliveryTimeRange, 
        pickupTime, rating, reviewCount, minOrderAmount, grade)
};

// ==========================================
// 메인 홈 응답 (CategoryHandler 용)
// ==========================================
struct MainHomeResDTO
{
    int status = 200;
    std::vector<CategoryItem> categories;              
    std::vector<BrandCategoryItem> brandCategories;    
    std::vector<TopStoreInfo> topStores;               

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(MainHomeResDTO, status, categories, brandCategories, topStores)
};

struct CategoryListResDTO
{
    int status = 200;                         
    std::vector<CategoryItem> categories; 

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(CategoryListResDTO, status, categories)
};

struct ReqGradeUpdateDTO
{
    std::string userId;
    std::string grade; 
    int action;        

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ReqGradeUpdateDTO, userId, grade, action)
};

struct ResGradeUpdateDTO
{
    int status ;
    std::string message;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ResGradeUpdateDTO, status, message)
};