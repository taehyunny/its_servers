#include "CategoryDAO.h"
#include "DbManager.h"
#include <iostream>
#include <mariadb/conncpp.hpp>

std::vector<CategoryItem> CategoryDAO::getAllCategories()
{
    std::vector<CategoryItem> categories;
    try
    {
        auto conn = DBManager::getInstance().getConnection();
        std::unique_ptr<sql::Statement> stmt(conn->createStatement());

        std::string query = "SELECT category_id, name, icon_name FROM CATEGORIES ORDER BY sort_order ASC";
        std::unique_ptr<sql::ResultSet> rs(stmt->executeQuery(query));

        while (rs->next())
        {
            // 🚀 CategoryItem 구조체에 맞춰서 담습니다.
            CategoryItem cat;    // DB 컬럼명과 DTO 필드명이 일치하도록 주의!
            cat.id = rs->getInt("category_id");  // DB 컬럼명과 DTO 필드명이 일치하도록 주의!
            cat.name = rs->getString("name");   
            cat.iconPath = rs->getString("icon_name"); 

            categories.push_back(cat);
        }
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "[CategoryDAO] 카테고리 로드 실패: " << e.what() << std::endl;
    }

    return categories;
}