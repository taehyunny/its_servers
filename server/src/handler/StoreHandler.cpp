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
        std::string targetCategory = "ALL";

        if (requestJson.contains("category"))
        {
            targetCategory = requestJson["category"].get<std::string>();
        }

        std::cout << "[StoreHandler] 상점 목록 요청 수신 - 카테고리: " << targetCategory << std::endl;

        StoreListResDTO resDto;
        resDto.status = 200;

        if (targetCategory == "ALL")
        {
            // 🚀 만약 ALL이 필요 없다면 그냥 이 if문을 통째로 지워도 됩니다.
            // 하지만 필요하다면 StoreDAO에 getAllStores()를 꼭 만들어두셔야 합니다! (아래 참고)
            resDto.stores = StoreDAO::getInstance().getAllStores();
        }
        else
        {
            resDto.stores = StoreDAO::getInstance().getStoresByCategory(targetCategory);
        }

        // 🚀 해결: 다시 JSON으로 예쁘게 포장해서 던집니다!
        nlohmann::json resJson = resDto;
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_STORE_LIST), resJson.dump());

        std::cout << "[StoreHandler] " << targetCategory << " 상점 " << resDto.stores.size() << "개 전송 완료." << std::endl;
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