#include "StoreHandler.h"
#include "StoreDAO.h"
#include "AllDTOs.h"
#include "Global_protocol.h"
#include <iostream>

void StoreHandler::handleStoreListRequest(ClientSession* session, const nlohmann::json& requestJson) {
    try {
        // 1. 클라이언트가 어떤 카테고리를 원하는지 파악 (없으면 "ALL"로 간주)
        std::string targetCategory = "ALL";
        if (requestJson.contains("category")) {
            targetCategory = requestJson["category"].get<std::string>();
        }

        std::cout << "[StoreHandler] 상점 목록 요청 수신 - 카테고리: " << targetCategory << std::endl;

        // 2. 상점 데이터 로드 (현재는 DAO 직접 호출, 추후 메모리 캐시 매니저로 교체)
        auto allStores = StoreDAO::getInstance().getAllStores();

        // 3. 응답 DTO 세팅 및 필터링
        StoreListResDTO resDto;
        resDto.status = 0;
        resDto.message = "상점 목록 조회 성공";

        for (const auto& store : allStores) {
            // 카테고리가 "ALL" 이거나, 요청한 카테고리와 일치하는 상점만 벡터에 담음
            if (targetCategory == "ALL" || store.category == targetCategory) {
                resDto.stores.push_back(store);
            }
        }

        // 4. JSON 직렬화 및 클라이언트로 전송
        nlohmann::json resJson = resDto;
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_STORE_LIST), resJson.dump());
        
        std::cout << "[StoreHandler] " << resDto.stores.size() << "개의 상점 전송 완료." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "[StoreHandler] 에러 발생: " << e.what() << std::endl;
        
        // 실패 시 에러 패킷 전송
        StoreListResDTO errorDto;
        errorDto.status = 1;
        errorDto.message = "서버 내부 오류로 상점 목록을 불러오지 못했습니다.";
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_STORE_LIST), nlohmann::json(errorDto).dump());
    }
}