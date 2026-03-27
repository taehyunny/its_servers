#pragma once
#include <vector>
#include <memory>
#include "AllDTOs.h"

struct MenuBasicInfo
{
    int storeId = 0;
    int basePrice = 0;
};
class MenuDAO
{
public:
    // 싱글톤 인스턴스 반환
    static MenuDAO &getInstance()
    {
        static MenuDAO instance; // 스레드 안전한 초기화 보장
        return instance;         // 싱글톤 패턴으로 객체 하나만 생성하여 반환
    }
    // 🛡️ [검증용 1] 메뉴의 진짜 가격과 상점 ID 가져오기
    MenuBasicInfo getMenuBasicInfo(int menuId);

    // 🛡️ [검증용 2] 옵션의 진짜 추가 가격 가져오기
    int getOptionPrice(int optionId);
    int getMenuBasePrice(int menuId);
    // 특정 상점(storeId)의 메뉴 목록만 DB에서 가져오는 함수
    std::vector<MenuDTO> getMenusByStoreId(int storeId);          // 메뉴 목록 가져오기
    std::vector<OptionGroup> getOptionGroupsByMenuId(int menuId); // 메뉴별 옵션 그룹 가져오기
    std::vector<OptionGroup> getMenuOptionsParsed(int menuId);    // 메뉴 옵션 JSON 파싱 후 반환
    std::string getMenuName(int menuId);
    std::string getOptionName(int menuId, int optionId); // 🚀 파라미터 2개로 변경

private:
    MenuDAO() = default;
    ~MenuDAO() = default;

    // 복사 방지
    MenuDAO(const MenuDAO &) = delete;
    MenuDAO &operator=(const MenuDAO &) = delete;
};