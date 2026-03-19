#include "MenuDAO.h"
#include "DbManager.h"
#include <iostream>
#include <mariadb/conncpp.hpp>

std::vector<MenuDTO> MenuDAO::getMenusByStoreId(int storeId) {
    std::vector<MenuDTO> menuList;

    try {
        // 1. 커넥션 풀에서 DB 연결 빌려오기
        auto conn = DBManager::getInstance().getConnection();
        
        // 2. PreparedStatement 준비 (SQL 인젝션 해킹 방지 및 성능 향상)
        // ? 자리에 나중에 storeId가 쏙 들어갑니다.
        std::unique_ptr<sql::PreparedStatement> pstmt(
            conn->prepareStatement("SELECT menu_id, menu_name, base_price, menu_options, "
                                   "is_sold_out, description, image_url, menu_category, is_popular "
                                   "FROM MENUS WHERE store_id = ?")
        );
        
        // ? 에 storeId 값을 세팅 (인덱스는 1부터 시작)
        pstmt->setInt(1, storeId);

        // 3. 쿼리 실행
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        // 4. 결과를 MenuDTO 벡터에 차곡차곡 담기
        while (res->next()) {
            MenuDTO menu;
            menu.menuId = res->getInt("menu_id");
            menu.menuName = res->getString("menu_name").c_str();
            menu.basePrice = res->getInt("base_price");
            menu.isSoldOut = res->getInt("is_sold_out");
            menu.description = res->getString("description").c_str();
            
            // NULL이 들어갈 수 있는 컬럼에 대한 안전장치
            if (res->isNull("image_url")) {
                menu.imageUrl = "";
            } else {
                menu.imageUrl = res->getString("image_url").c_str();
            }

            menu.menuCategory = res->getString("menu_category").c_str();
            menu.isPopular = res->getBoolean("is_popular"); // TINYINT(1)은 boolean으로 매핑됨

            // 🚀 Qt에서 UI 위젯을 만들어줄 핵심! JSON 옵션 파싱
            std::string optionsStr = res->getString("menu_options").c_str();
            if (!optionsStr.empty()) {
                try {
                    menu.menuOptions = nlohmann::json::parse(optionsStr);
                } catch (...) {
                    menu.menuOptions = nlohmann::json::object(); // 파싱 에러 시 빈 객체로 방어
                }
            } else {
                menu.menuOptions = nlohmann::json::object();
            }

            menuList.push_back(menu);
        }
        std::cout << "[MenuDAO] 상점 ID " << storeId << "의 메뉴 " << menuList.size() << "개를 로드했습니다." << std::endl;

    } catch (sql::SQLException& e) {
        std::cerr << "[FATAL] MenuDAO::getMenusByStoreId DB 에러: " << e.what() << std::endl;
    }

    return menuList;
}