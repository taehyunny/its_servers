#pragma once
#include <vector>
#include <string>
#include "AllDTOs.h"

class AdminDAO
{
public:
    // 🚀 싱글톤 패턴 적용
    static AdminDAO &getInstance()
    {
        static AdminDAO instance;
        return instance;
    }

    // 👑 관리자 전용: 조건부 전체 주문 검색
    std::vector<AdminOrderDTO> searchOrders(const std::string &searchType, const std::string &keyword);

private:
    AdminDAO() = default;
    ~AdminDAO() = default;
};