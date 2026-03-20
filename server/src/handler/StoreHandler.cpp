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
        // 1. 클라이언트가 보낸 JSON 문자열을 파싱
        json requestJson = json::parse(jsonBody);
        std::string targetCategory = "ALL";

        if (requestJson.contains("category"))
        {
            targetCategory = requestJson["category"].get<std::string>();
        }

        std::cout << "[StoreHandler] 상점 목록 요청 수신 - 카테고리: " << targetCategory << std::endl;

        // 2. 응답 DTO 세팅
        StoreListResDTO resDto;
        resDto.status = 200; // 성공 코드는 200이 국룰입니다! ㅋㅋㅋ
        // resDto.message = "상점 목록 조회 성공"; // DTO에 message 필드가 있다면 주석 해제

        // 🚀 3. 인메모리 필터링 대신 DB 쿼리로 즉시 가져오기! (속도 극대화)
        if (targetCategory == "ALL")
        {
            // "ALL"일 때는 전체를 가져오는 DAO 메서드 사용
            resDto.stores = StoreDAO::getInstance().getAllStores();
        }
        else
        {
            // 🚀 우리가 아까 만든 "매출 순"으로 정렬해서 가져오는 쿼리 사용!
            resDto.stores = StoreDAO::getInstance().getStoresByCategory(targetCategory);
        }

        // 4. 클라이언트로 전송
        // (session->sendPacket이 DTO를 바로 받을 수 있게 오버로딩 되어 있다면 이렇게 한 줄로 끝납니다!)
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_STORE_LIST), resDto);

        std::cout << "[StoreHandler] " << targetCategory << " 상점 " << resDto.stores.size() << "개 전송 완료." << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "[StoreHandler] 에러 발생: " << e.what() << std::endl;

        // 실패 시 에러 패킷 전송
        StoreListResDTO errorDto;
        errorDto.status = 500; // 서버 에러 코드
        // errorDto.message = "서버 내부 오류로 상점 목록을 불러오지 못했습니다.";

        session->sendPacket(static_cast<uint16_t>(CmdID::RES_STORE_LIST), errorDto);
    }
}