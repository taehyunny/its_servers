#include "MenuHandler.h"
#include "MenuDAO.h"
#include "AllDTOs.h"
#include "Global_protocol.h"
#include <iostream>
#include <stdexcept>

void MenuHandler::handleMenuListRequest(ClientSession* session, const nlohmann::json& requestJson) {
    try {
        // 1. 객관적 팩트 체크: 클라이언트가 storeId를 제대로 보냈는가?
        if (!requestJson.contains("storeId")) {
            throw std::invalid_argument("요청 패킷에 'storeId'가 누락되었습니다.");
        }

        int targetStoreId = requestJson["storeId"].get<int>();
        std::cout << "[MenuHandler] 상점 ID " << targetStoreId << "의 메뉴 목록 요청 수신" << std::endl;

        // 2. MenuDAO를 통해 해당 상점의 메뉴 데이터 쫙 빨아오기
        auto menus = MenuDAO::getInstance().getMenusByStoreId(targetStoreId);

        // 3. 응답 DTO 조립 (AllDTOs.h의 MenuListResDTO 구조체 활용)
        MenuListResDTO resDto;
        resDto.status = 0;
        resDto.message = "메뉴 목록 조회 성공";
        resDto.storeId = targetStoreId;
        resDto.menus = menus; // std::vector 통째로 복사

        // 4. JSON 직렬화 및 클라이언트로 전송 (static_cast 갑옷 착용)
        nlohmann::json resJson = resDto;
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_MENU_LIST), resJson.dump());
        
        std::cout << "[MenuHandler] " << resDto.menus.size() << "개의 메뉴 전송 완료." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "[MenuHandler] 에러 발생: " << e.what() << std::endl;
        
        // 실패 시 에러 패킷 전송
        MenuListResDTO errorDto;
        errorDto.status = 1;
        errorDto.message = std::string("메뉴 목록 로드 실패: ") + e.what();
        errorDto.storeId = -1; // 에러 상황이므로 임시 값 대입
        
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_MENU_LIST), nlohmann::json(errorDto).dump());
    }
}