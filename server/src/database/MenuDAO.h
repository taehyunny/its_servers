#pragma once
#include <vector>
#include <memory>
#include "AllDTOs.h"

class MenuDAO
{
public:
    // 싱글톤 인스턴스 반환
    static MenuDAO &getInstance()
    {
        static MenuDAO instance; // 스레드 안전한 초기화 보장
        return instance;         // 싱글톤 패턴으로 객체 하나만 생성하여 반환
    }

    // 특정 상점(storeId)의 메뉴 목록만 DB에서 가져오는 함수
    std::vector<MenuDTO> getMenusByStoreId(int storeId);   // 메뉴 목록 가져오기
    std::vector<OptionGroup> getOptionGroupsByMenuId(int menuId); // 메뉴별 옵션 그룹 가져오기

private:
    MenuDAO() = default;
    ~MenuDAO() = default;

    // 복사 방지
    MenuDAO(const MenuDAO &) = delete;
    MenuDAO &operator=(const MenuDAO &) = delete;
};