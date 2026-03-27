#include "MenuDAO.h"
#include "DbManager.h"
#include "AllDTOs.h"
#include <iostream>
#include <string>
#include <vector>
#include <algorithm> // 🚀 std::sort를 위해 필수
#include <mariadb/conncpp.hpp>

// ── 1. 특정 상점의 메뉴 목록 조회 ──────────────────────────────────
std::vector<MenuDTO> MenuDAO::getMenusByStoreId(int storeId)
{
    std::vector<MenuDTO> menuList;

    try
    {
        auto conn = DBManager::getInstance().getConnection();

        // 🚀 SQL 쿼리에서 더 이상 menu_options(JSON)를 읽지 않고 개별 테이블을 활용합니다.
        std::unique_ptr<sql::PreparedStatement> pstmt(
            conn->prepareStatement("SELECT menu_id, menu_name, base_price, is_sold_out, "
                                   "description, image_url, menu_category, is_popular "
                                   "FROM MENUS WHERE store_id = ?"));

        pstmt->setInt(1, storeId);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        while (res->next())
        {
            MenuDTO menu;
            menu.menuId = res->getInt("menu_id");
            menu.menuName = res->getString("menu_name").c_str();
            menu.basePrice = res->getInt("base_price");
            menu.isSoldOut = res->getInt("is_sold_out");
            menu.isPopular = res->getBoolean("is_popular");
            menu.description = res->isNull("description") ? "" : res->getString("description").c_str();
            menu.imageUrl = res->isNull("image_url") ? "" : res->getString("image_url").c_str();
            menu.menuCategory = res->isNull("menu_category") ? "기본 메뉴" : res->getString("menu_category").c_str();

            // 🚀 핵심: 기존 JSON 파싱 대신, 아래에 만든 계층형 로드 함수를 호출합니다!
            menu.optionGroups = getOptionGroupsByMenuId(menu.menuId);

            menuList.push_back(menu);
        }
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "[MenuDAO] getMenusByStoreId 에러: " << e.what() << std::endl;
    }

    return menuList;
}

// ── 2. 메뉴별 옵션 그룹 및 세부 항목 로드 (계층형 + 정렬) ────────────
std::vector<OptionGroup> MenuDAO::getOptionGroupsByMenuId(int menuId)
{
    std::vector<OptionGroup> groupList;
    try
    {
        auto conn = DBManager::getInstance().getConnection();

        // [Step 1] 옵션 그룹(큰 틀) 조회
        std::unique_ptr<sql::PreparedStatement> pstmtGroup(conn->prepareStatement(
            "SELECT group_id, group_name, is_required, max_count, display_order "
            "FROM MENU_OPTION_GROUPS WHERE menu_id = ?"));
        pstmtGroup->setInt(1, menuId);
        std::unique_ptr<sql::ResultSet> rsGroup(pstmtGroup->executeQuery());

        while (rsGroup->next())
        {
            OptionGroup group;
            group.groupId = rsGroup->getInt("group_id");
            group.groupName = rsGroup->getString("group_name").c_str();
            group.isRequired = rsGroup->getBoolean("is_required"); // 🚀 필수 선택 여부
            group.maxCount = rsGroup->getInt("max_count");
            group.displayOrder = rsGroup->getInt("display_order");

            // [Step 2] 각 그룹에 속한 세부 옵션 항목(작은 틀) 조회
            std::unique_ptr<sql::PreparedStatement> pstmtItem(conn->prepareStatement(
                "SELECT option_id, option_name, additional_price, display_order "
                "FROM MENU_OPTIONS WHERE group_id = ?"));
            pstmtItem->setInt(1, group.groupId);
            std::unique_ptr<sql::ResultSet> rsItem(pstmtItem->executeQuery());

            while (rsItem->next())
            {
                OptionItem item;
                item.optionId = rsItem->getInt("option_id");
                item.optionName = rsItem->getString("option_name").c_str();
                item.additionalPrice = rsItem->getInt("additional_price");
                item.displayOrder = rsItem->getInt("display_order");

                group.options.push_back(item); // 🚀 하위 벡터에 추가
            }

            // [Step 3] 🚀 정렬 알고리즘 적용: 그룹 내 옵션들을 display_order 순으로 정렬
            std::sort(group.options.begin(), group.options.end(), [](const OptionItem &a, const OptionItem &b)
                      { return a.displayOrder < b.displayOrder; });

            groupList.push_back(group);
        }

        // [Step 4] 🚀 정렬 알고리즘 적용: 옵션 그룹들 자체를 display_order 순으로 정렬
        std::sort(groupList.begin(), groupList.end(), [](const OptionGroup &a, const OptionGroup &b)
                  { return a.displayOrder < b.displayOrder; });
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "[MenuDAO] getOptionGroupsByMenuId 에러: " << e.what() << std::endl;
    }
    return groupList;
}

// ── 🛡️ [검증용 1] 메뉴 원가 및 상점 ID 조회 ──────────────────────────
MenuBasicInfo MenuDAO::getMenuBasicInfo(int menuId)
{
    MenuBasicInfo info;
    try
    {
        auto conn = DBManager::getInstance().getConnection();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            conn->prepareStatement("SELECT store_id, base_price FROM MENUS WHERE menu_id = ?"));
        pstmt->setInt(1, menuId);
        std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());

        if (rs->next())
        {
            info.storeId = rs->getInt("store_id");
            info.basePrice = rs->getInt("base_price");
        }
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "[MenuDAO] MenuBasicInfo 조회 실패: " << e.what() << std::endl;
    }
    return info;
}

// ── 🛡️ [검증용 2] 개별 옵션 가격 조회 ──────────────────────────────
int MenuDAO::getOptionPrice(int optionId)
{
    int price = 0;
    try
    {
        auto conn = DBManager::getInstance().getConnection();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            conn->prepareStatement("SELECT additional_price FROM MENU_OPTIONS WHERE option_id = ?"));
        pstmt->setInt(1, optionId);
        std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());

        if (rs->next())
        {
            price = rs->getInt("additional_price");
        }
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "[MenuDAO] OptionPrice 조회 실패: " << e.what() << std::endl;
    }
    return price;
}
int MenuDAO::getMenuBasePrice(int menuId)
{
    try
    {
        auto conn = DBManager::getInstance().getConnection();
        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
            "SELECT base_price FROM MENUS WHERE menu_id = ?"));
        pstmt->setInt(1, menuId);
        std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());

        if (rs->next())
            return rs->getInt("base_price");
    }
    catch (...)
    {
    }
    return -1; // 실패 시 -1 반환
}

std::vector<OptionGroup> MenuDAO::getMenuOptionsParsed(int menuId)
{
    std::vector<OptionGroup> resultGroups;
    try
    {
        auto conn = DBManager::getInstance().getConnection();
        if (!conn)
            return resultGroups;

        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
            "SELECT menu_options FROM MENUS WHERE menu_id = ?"));
        pstmt->setInt(1, menuId);

        std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());

        if (rs->next())
        {
            std::string optionsStr = rs->isNull("menu_options") ? "[]" : rs->getString("menu_options").c_str();

            // 🚀 태현님이 구현해둔 from_json 마법이 발동되는 순간!
            // JSON 문자열을 파싱해서 std::vector<OptionGroup>에 한 번에 쏟아 붓습니다.
            nlohmann::json j = nlohmann::json::parse(optionsStr);
            j.get_to(resultGroups);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [MenuDAO] 메뉴 옵션 파싱 에러: " << e.what() << std::endl;
    }

    return resultGroups;
}
std::string MenuDAO::getMenuName(int menuId)
{
    std::cout << ">>> [DEBUG] getMenuName 호출됨! 넘어온 menuId: " << menuId << std::endl;
    auto conn = DBManager::getInstance().getConnection();
    try
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
            "SELECT menu_name FROM MENUS WHERE menu_id = ?"));
        pstmt->setInt(1, menuId);
        std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());

        if (rs->next())
        {
            return rs->getString("menu_name").c_str(); // 🚀 찐 메뉴 이름 리턴!
        }
    }
    catch (const sql::SQLException &e)
    {
        std::cerr << "🚨 [MenuDAO] 메뉴 이름 조회 에러: " << e.what() << std::endl;
    }

    return ""; // 못 찾으면 빈 문자열 리턴
}
// MenuDAO.cpp 하단에 추가

// [MenuDAO.cpp 내부]
std::string MenuDAO::getOptionName(int menuId, int optionId)
{
    auto conn = DBManager::getInstance().getConnection();
    try
    {
        std::cout << ">>> [DEBUG][MenuDAO] 🔍 옵션 이름 조회 시도... (메뉴: " << menuId << ", 옵션: " << optionId << ")" << std::endl;

        // 🚀 1. MENUS 테이블에서 해당 메뉴의 JSON(menu_options)을 통째로 가져옵니다.
        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
            "SELECT menu_options FROM MENUS WHERE menu_id = ?"));
        pstmt->setInt(1, menuId);
        std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());

        if (rs->next())
        {
            std::string jsonStr = rs->isNull("menu_options") ? "" : rs->getString("menu_options").c_str();

            if (!jsonStr.empty())
            {
                // 🚀 2. JSON 파싱 시작 (올려주신 사진의 구조대로 껍질을 깝니다)
                nlohmann::json groups = nlohmann::json::parse(jsonStr);

                if (groups.is_array())
                {
                    for (const auto &group : groups)
                    {
                        auto optsArray = group.value("options", nlohmann::json::array());
                        for (const auto &opt : optsArray)
                        {
                            // 💡 우리가 찾는 optionId와 일치하면 이름을 리턴!
                            if (opt.value("optionId", 0) == optionId)
                            {
                                std::string optName = opt.value("optionName", "");
                                std::cout << ">>> [DEBUG][MenuDAO] ✅ 옵션 찾음! (ID: " << optionId << " -> 이름: [" << optName << "])" << std::endl;
                                return optName;
                            }
                        }
                    }
                }
            }
        }

        std::cout << ">>> [DEBUG][MenuDAO] ⚠️ 옵션 찾기 실패! 메뉴 " << menuId << "의 JSON 안에 option_id(" << optionId << ")가 없습니다." << std::endl;
    }
    catch (const nlohmann::json::parse_error &e)
    {
        std::cerr << "🚨 [MenuDAO] JSON 파싱 에러 (DB 데이터 오염 의심): " << e.what() << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [MenuDAO] 옵션 이름 조회 일반 에러: " << e.what() << std::endl;
    }

    return ""; // 못 찾으면 빈 문자열 리턴
}