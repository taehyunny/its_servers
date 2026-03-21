#include "StoreHandler.h"
#include "StoreDAO.h"
#include "AllDTOs.h"
#include "Global_protocol.h"
#include <iostream>

using nlohmann::json;

void StoreHandler::handleStoreListRequest(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        json requestJson = json::parse(jsonBody);

        // 🚀 1. 클라이언트가 보낸 categoryId (int)를 읽습니다.
        // 만약 필드가 없으면 기본값 0(전체)으로 설정합니다.
        int categoryId = requestJson.value("categoryId", 0);

        std::cout << "[StoreHandler] 상점 목록 요청 수신 - 카테고리 ID: " << categoryId << std::endl;

        StoreListResDTO resDto;
        resDto.status = 200;

        // 🚀 2. ID가 0이면 전체 조회, 아니면 우리가 방금 만든 ID 기반 함수 호출!
        if (categoryId == 0)
        {
            resDto.stores = StoreDAO::getInstance().getAllStores();
        }
        else
        {
            resDto.stores = StoreDAO::getInstance().getStoresByCategoryId(categoryId);
        }

        // 🚀 3. 결과를 JSON으로 포장해서 전송
        nlohmann::json debugJson = resDto;
        std::cout << ">>> [DEBUG] 전송할 JSON 데이터:\n"
                  << debugJson.dump(4) << std::endl;

        // 2. 🚀 핵심: dump() 문자열이 아닌 '객체(resDto)'를 직접 던지기!
        // 이렇게 하면 이중 직렬화 문제 없이 클라이언트가 바로 읽을 수 있습니다.
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_STORE_LIST), resDto);

        std::cout << "[StoreHandler] 전송 완료." << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "[StoreHandler] 에러 발생: " << e.what() << std::endl;

        StoreListResDTO errorDto;
        errorDto.status = 500;

        // 🚀 에러 패킷도 똑같이 JSON으로 포장!
        nlohmann::json errorJson = errorDto;
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_STORE_LIST), errorJson.dump());
    }
}