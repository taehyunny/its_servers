#pragma once
#include <vector>
#include <string>
#include "AllDTOs.h"

class StoreDAO
{
public:
    static StoreDAO &getInstance()
    {
        static StoreDAO instance;
        return instance;
    }

    // 🚀 1. 메인 화면용: 각 카테고리별 매출 1등 매장들만 가져오기
    std::vector<TopStoreInfo> getTopStoresByCategory();

    std::vector<TopStoreInfo> getAllStores();
    // 🚀 2. 상세 화면용: 특정 카테고리의 모든 매장을 매출순으로 가져오기
    std::vector<TopStoreInfo> getStoresByCategory(int categoryId, const std::string &brandName);
    std::vector<TopStoreInfo> getStoresByCategoryId(int categoryId, const std::string &brandName);
    bool executeUpdate(const std::string &query, const std::vector<std::string> &params);
    ResStoreDetailDTO getStoreDetail(int storeId);
    int getDeliveryFee(int storeId);
    std::string getOwnerIdByStoreId(int storeId);
    // 매장 ID를 넣으면 배달비(int)를 뱉어내는 함수 선언

private:
    StoreDAO() = default;
    ~StoreDAO() = default;
    StoreDAO(const StoreDAO &) = delete;
    StoreDAO &operator=(const StoreDAO &) = delete;
};