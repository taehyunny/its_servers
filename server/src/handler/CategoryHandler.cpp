#include "CategoryHandler.h"
#include "CategoryDAO.h"
#include "StoreDAO.h"
#include "AllDTOs.h"
#include "Global_protocol.h"
#include "DbManager.h"

#include <iostream>

void CategoryHandler::handleCategoryRequest(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    std::cout << "[CategoryHandler] 클라이언트 메인 화면 통합 데이터 요청!" << std::endl;

    try
    {
        auto conn = DBManager::getInstance().getConnection();
        MainHomeResDTO res;
        res.status = 200;

        // ---------------------------------------------------------
        // 🚀 1. 윗줄: 음식 카테고리 ('편의점', '마트' 제외)
        // ---------------------------------------------------------
        std::unique_ptr<sql::PreparedStatement> pstmtFood(conn->prepareStatement(
            "SELECT category_id, name, icon_name FROM CATEGORIES WHERE name NOT IN ('편의점', '마트') ORDER BY category_id ASC"));
        std::unique_ptr<sql::ResultSet> rsFood(pstmtFood->executeQuery());

        while (rsFood->next())
        {
            CategoryItem item;
            item.id = rsFood->getInt("category_id");
            item.name = rsFood->isNull("name") ? "" : std::string(rsFood->getString("name"));
            item.iconPath = rsFood->isNull("icon_name") ? "" : std::string(rsFood->getString("icon_name"));
            res.categories.push_back(item);
        }

        // ---------------------------------------------------------
        // 🚀 2. 아랫줄: 브랜드 카테고리 (Git에서 날아갔던 부분 복구!)
        // ---------------------------------------------------------
        std::unique_ptr<sql::PreparedStatement> pstmtBrand(conn->prepareStatement(
            "SELECT DISTINCT brand_name FROM STORES WHERE category IN ('편의점', '마트') AND brand_name IS NOT NULL"));
        std::unique_ptr<sql::ResultSet> rsBrand(pstmtBrand->executeQuery());

        while (rsBrand->next())
        {
            BrandCategoryItem item;
            item.brandName = std::string(rsBrand->getString("brand_name"));
            res.brandCategories.push_back(item);
        }

        // ---------------------------------------------------------
        // 🚀 3. 1등 매장 리스트 (StoreDAO에서 쓰레기값 제거된 안전한 데이터)
        // ---------------------------------------------------------
        res.topStores = StoreDAO::getInstance().getTopStoresByCategory();

        // ---------------------------------------------------------
        // 🚀 4. 직렬화 (DTO -> JSON) 후 전송
        // ---------------------------------------------------------
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_CATEGORY), nlohmann::json(res));

        std::cout << "[CategoryHandler] ✅ 메인 화면 데이터 전송 완료!\n"
                  << " 👉 음식 카테고리: " << res.categories.size() << "개\n"
                  << " 👉 브랜드 카테고리: " << res.brandCategories.size() << "개\n"
                  << " 👉 1등 매장: " << res.topStores.size() << "개" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [CategoryHandler] 메인 화면 전송 에러: " << e.what() << std::endl;

        MainHomeResDTO errorRes;
        errorRes.status = 500;
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_CATEGORY), nlohmann::json(errorRes));
    }
}