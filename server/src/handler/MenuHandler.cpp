#include "MenuHandler.h"
#include "ClientSession.h"
#include "Global_protocol.h"
#include "DbManager.h"
#include "StoreDAO.h"
#include "MenuDAO.h" // 메뉴 관련 DB 작업을 위한 DAO
#include "AllDTOs.h" // 요청/응답 DTO 정의
#include <nlohmann/json.hpp>
#include <iostream>
#include <vector>

using nlohmann::json;

// ── ① 메뉴 목록 조회 (RES_MENU_LIST = 2011) ──
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
            "menu_category, is_popular, description, image_url, menu_options " // 🚀 menu_options 컬럼 추가
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

            // 🚀 [수정 사항 1] menuOptions -> optionGroups 키 명칭 변경 및 JSON 파싱
            std::string menuOptionsStr = rs->isNull("menu_options") ? "[]" : rs->getString("menu_options").c_str();
            try
            {
                menu["optionGroups"] = json::parse(menuOptionsStr);
            }
            catch (...)
            {
                menu["optionGroups"] = json::array(); // 파싱 실패 시 빈 배열로 안전하게 처리
            }

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

// ── ② 메뉴 추가/수정/삭제 (RES_MENU_EDIT = 2013) ──
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
                "menu_category, is_popular, description, is_sold_out, menu_options) "
                "VALUES (?, ?, ?, ?, ?, ?, 0, ?)"));

            pstmt->setInt(1, storeId);
            pstmt->setString(2, menuData.value("menuName", ""));
            pstmt->setInt(3, menuData.value("basePrice", 0));
            pstmt->setString(4, menuData.value("menuCategory", "기본 메뉴"));
            pstmt->setBoolean(5, menuData.value("isPopular", false));
            pstmt->setString(6, menuData.value("description", ""));

            // 🚀 [수정 사항 2] 저장 키 명칭 변경: menuOptions -> optionGroups
            std::string menuOptionsStr = menuData.contains("optionGroups")
                                             ? menuData["optionGroups"].dump()
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
                "menu_category=?, is_popular=?, description=?, is_sold_out=?, menu_options=? "
                "WHERE menu_id=? AND store_id=?"));

            pstmt->setString(1, menuData.value("menuName", ""));
            pstmt->setInt(2, menuData.value("basePrice", 0));
            pstmt->setString(3, menuData.value("menuCategory", "기본 메뉴"));
            pstmt->setBoolean(4, menuData.value("isPopular", false));
            pstmt->setString(5, menuData.value("description", ""));
            pstmt->setBoolean(6, menuData.value("isSoldOut", false));

            // 🚀 [수정 사항 2] 저장 키 명칭 변경: menuOptions -> optionGroups
            std::string menuOptionsStr = menuData.contains("optionGroups")
                                             ? menuData["optionGroups"].dump()
                                             : "[]";
            pstmt->setString(7, menuOptionsStr);

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

// ── ③ 품절 처리 (RES_MENU_SOLD_OUT = 2012) ──
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
void MenuHandler::handleMenuOption(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        // 1. 클라이언트 요청 파싱 (menuId 추출)
        auto req = nlohmann::json::parse(jsonBody).get<ReqMenuOptionDTO>();
        
        // 2. DAO를 통해 파싱된 옵션 객체 배열 가져오기
        std::vector<OptionGroup> groups = MenuDAO::getInstance().getMenuOptionsParsed(req.menuId);

        // 3. 응답 DTO 세팅
        ResMenuOptionDTO res;
        res.status = 200;
        res.menuId = req.menuId;
        res.optionGroups = groups;

        // 4. 전송! (태현님이 짜둔 to_json이 알아서 JSON으로 예쁘게 바꿔줍니다)
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_MENU_OPTION), nlohmann::json(res));
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [MenuHandler] handleMenuOption 에러: " << e.what() << std::endl;
        ResMenuOptionDTO errRes;
        errRes.status = 500;
        errRes.menuId = 0;
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_MENU_OPTION), nlohmann::json(errRes));
    }
}