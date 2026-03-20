#pragma once
#include <vector>
#include <memory>
#include "AllDTOs.h" // CMake에서 경로를 잡았으므로 파일명만 적습니다.

class StoreDAO
{
public:
    // 싱글톤 인스턴스 반환
    static StoreDAO &getInstance()
    {
        static StoreDAO instance;
        return instance;
    }

    // 서버 시작 시 캐싱을 위해 모든 상점 정보를 DB에서 가져오는 함수
    std::vector<TopStoreInfo> getAllStores();
    std::vector<TopStoreInfo> getStoresByCategory(const std::string &category);
    std::vector<TopStoreInfo> getStoresByCategory(const std::string &categoryName);

private:
    StoreDAO() = default;
    ~StoreDAO() = default;

    // 복사 방지
    StoreDAO(const StoreDAO &) = delete;
    StoreDAO &operator=(const StoreDAO &) = delete;
};