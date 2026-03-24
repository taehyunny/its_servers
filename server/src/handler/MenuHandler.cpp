#include "MenuHandler.h"
#include "ClientSession.h"
#include "Global_protocol.h"
#include "DbManager.h"
#include "StoreDAO.h" // 🚀 UserDAO 대신 안전한 범용 업데이트를 위해 추가
#include <nlohmann/json.hpp>
#include <iostream>
#include <vector>

using nlohmann::json;

// ① 메뉴 목록 조회
void MenuHandler::handleMenuListRequest(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    json req = json::parse(jsonBody);
    json res;
    try
    {
        int storeId = req.value("storeId", 0);

        auto conn = DBManager::getInstance().getConnection();
        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
            "SELECT menu_id, menu_name, base_price, is_sold_out, "
            "menu_category, is_popular, description, image_url "
            "FROM MENUS WHERE store_id = ?"));
        pstmt->setInt(1, storeId);
        std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());

        json menus = json::array();
        while (rs->next())
        {
            json menu;
            menu["menuId"] = rs->getInt("menu_id");
            menu["menuName"] = rs->getString("menu_name").c_str();
            menu["basePrice"] = rs->getInt("base_price");
            menu["isSoldOut"] = rs->getBoolean("is_sold_out");
            menu["menuCategory"] = rs->getString("menu_category").c_str();
            menu["isPopular"] = rs->getBoolean("is_popular");
            menu["description"] = rs->isNull("description") ? "" : rs->getString("description").c_str();
            menu["imageUrl"] = rs->isNull("image_url") ? "" : rs->getString("image_url").c_str();
            menus.push_back(menu);
        }

        res["status"] = 200;
        res["message"] = "조회 성공";
        res["storeId"] = storeId;
        res["menus"] = menus;

        session->sendPacket(static_cast<uint16_t>(CmdID::RES_MENU_LIST), res);
    }
    catch (const std::exception &e)
    {
        std::cerr << "[MenuHandler] List Error: " << e.what() << std::endl;
        res["status"] = 500;
        res["message"] = "서버 내부 오류";
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_MENU_LIST), res);
    }
}

// ② 메뉴 추가/수정/삭제
void MenuHandler::handleMenuEdit(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    std::cout << "[MenuEdit Debug] Received JSON: " << jsonBody << std::endl;
    json req = json::parse(jsonBody);
    json res;
    try
    {
        int storeId = req.value("storeId", 0);
        int actionType = req.value("actionType", -1);
        auto menuData = req["menuData"];

        bool ok = false;
        auto conn = DBManager::getInstance().getConnection();

        if (actionType == 0) // ── 추가 ──
        {
            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
                "INSERT INTO MENUS (store_id, menu_name, base_price, "
                "menu_category, is_popular, description, is_sold_out, menu_options) " // 🚀 menu_options 추가됨
                "VALUES (?, ?, ?, ?, ?, ?, 0, ?)"));                                  // 🚀 ? 1개 추가됨

            pstmt->setInt(1, storeId);
            pstmt->setString(2, menuData.value("menuName", ""));
            pstmt->setInt(3, menuData.value("basePrice", 0));
            pstmt->setString(4, menuData.value("menuCategory", "기본 메뉴"));
            pstmt->setBoolean(5, menuData.value("isPopular", false));
            pstmt->setString(6, menuData.value("description", ""));

            // 🚀 프론트 요청사항: 옵션 배열 파싱 후 7번째 파라미터로 넣기
            std::string menuOptionsStr = menuData.contains("menuOptions")
                                             ? menuData["menuOptions"].dump()
                                             : "[]";
            pstmt->setString(7, menuOptionsStr);

            pstmt->executeUpdate();
            ok = true;
        }
        else if (actionType == 1) // ── 수정 ──
        {
            int menuId = menuData.value("menuId", 0);
            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
                "UPDATE MENUS SET menu_name=?, base_price=?, "
                "menu_category=?, is_popular=?, description=?, is_sold_out=?, menu_options=? " // 🚀 menu_options=? 추가됨
                "WHERE menu_id=? AND store_id=?"));

            pstmt->setString(1, menuData.value("menuName", ""));
            pstmt->setInt(2, menuData.value("basePrice", 0));
            pstmt->setString(3, menuData.value("menuCategory", "기본 메뉴"));
            pstmt->setBoolean(4, menuData.value("isPopular", false));
            pstmt->setString(5, menuData.value("description", ""));
            pstmt->setBoolean(6, menuData.value("isSoldOut", false));

            // 🚀 프론트 요청사항: 7번째 파라미터로 옵션 JSON 문자열 넣기
            std::string menuOptionsStr = menuData.contains("menuOptions")
                                             ? menuData["menuOptions"].dump()
                                             : "[]";
            pstmt->setString(7, menuOptionsStr);

            // 🚀 기존 7, 8번이었던 파라미터가 8, 9번으로 밀려남
            pstmt->setInt(8, menuId);
            pstmt->setInt(9, storeId);

            pstmt->executeUpdate();
            ok = true;
        }
        else if (actionType == 2) // ── 삭제 ──
        {
            int menuId = menuData.value("menuId", 0);
            std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
                "DELETE FROM MENUS WHERE menu_id=? AND store_id=?"));
            pstmt->setInt(1, menuId);
            pstmt->setInt(2, storeId);
            pstmt->executeUpdate();
            ok = true;
        }

        res["status"] = ok ? 200 : 400;
        res["message"] = ok ? "메뉴 변경 완료" : "잘못된 actionType";
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_MENU_EDIT), res);
    }
    catch (const std::exception &e)
    {
        std::cerr << "[MenuHandler] Edit Error: " << e.what() << std::endl;
        res["status"] = 500;
        res["message"] = "서버 내부 오류";
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_MENU_EDIT), res);
    }
}

// ③ 품절 처리
void MenuHandler::handleMenuSoldOut(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    json req = json::parse(jsonBody);
    json res;
    try
    {
        int menuId = req.value("menuId", 0);
        bool isSoldOut = req.value("isSoldOut", false);

        std::string query = "UPDATE MENUS SET is_sold_out=? WHERE menu_id=?";
        std::vector<std::string> params = {
            isSoldOut ? "1" : "0",
            std::to_string(menuId)};

        // 🚀 핵심: 팀원분의 UserDAO 오타를 StoreDAO로 교체! (안전한 업데이트)
        bool ok = StoreDAO::getInstance().executeUpdate(query, params);

        res["status"] = ok ? 200 : 500;
        res["message"] = ok ? "품절 상태 변경 완료" : "DB 오류";
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_MENU_SOLD_OUT), res);
    }
    catch (const std::exception &e)
    {
        std::cerr << "[MenuHandler] SoldOut Error: " << e.what() << std::endl;
        res["status"] = 500;
        res["message"] = "서버 내부 오류";
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_MENU_SOLD_OUT), res);
    }
}