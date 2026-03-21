#include "MenuHandler.h"
#include "MenuDAO.h"
#include "AllDTOs.h"
#include "Global_protocol.h"
#include <iostream>

void MenuHandler::handleMenuListRequest(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        // 1. 클라이언트의 요청 파싱 (storeId 추출)
        auto req = nlohmann::json::parse(jsonBody).get<MenuListReqDTO>();
        int targetStoreId = req.storeId;

        std::cout << "[MenuHandler] 클라이언트가 매장 ID " << targetStoreId << "의 메뉴 리스트를 요청했습니다." << std::endl;

        // 2. 응답 DTO 준비
        MenuListResDTO res;

        // 🚀 3. 태현님이 만든 완벽한 DAO 호출!
        res.menus = MenuDAO::getInstance().getMenusByStoreId(targetStoreId);

        // 결과에 따른 상태 코드 및 메시지 세팅
        if (res.menus.empty())
        {
            res.status = 404; // 데이터가 없으면 404 Not Found
            res.message = "등록된 메뉴가 없습니다.";
        }
        else
        {
            res.status = 200;
            res.message = "메뉴 목록 조회 성공";
        }

        // 4. 클라이언트로 패킷 발사 (RES_MENU_LIST = 2011)
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_MENU_LIST), res);

        std::cout << "[MenuHandler] 매장 ID " << targetStoreId << "의 메뉴 " << res.menus.size() << "개 전송 완료!" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "[MenuHandler] 에러 발생: " << e.what() << std::endl;

        // 에러 발생 시 클라이언트에게 에러 패킷 전송
        MenuListResDTO errorRes;
        errorRes.status = 500;
        errorRes.message = "서버 내부 오류로 메뉴를 불러오지 못했습니다.";

        session->sendPacket(static_cast<uint16_t>(CmdID::RES_MENU_LIST), errorRes);
    }
}