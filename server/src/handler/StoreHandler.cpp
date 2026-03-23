#include "StoreHandler.h"
#include "StoreDAO.h"
#include "AllDTOs.h"
#include "UserDAO.h"
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

void StoreHandler::handleStoreInfoUpdateReq(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        // 클라이언트가 보낸 JSON 문자열을 객체로 파싱
        json req = json::parse(jsonBody);

        // ── 1단계: storeId 확인 (필수값) ──────────────────────
        int storeId = req.value("storeId", 0);
        if (storeId == 0)
        {
            json res;
            res["status"] = 400;
            res["message"] = "storeId가 없습니다.";
            session->sendPacket(static_cast<int>(CmdID::RES_STORE_INFO_UPDATE), res);
            return;
        }

        bool success = true;

        // ── 2단계: STORES 테이블 변경 필드 조립 ───────────────
        std::string storeClauses;
        std::vector<std::string> storeValues;

        if (req.contains("storeName"))
        {
            storeClauses += "store_name=?, ";
            storeValues.push_back(req["storeName"].get<std::string>());
        }
        if (req.contains("category"))
        {
            storeClauses += "category=?, ";
            storeValues.push_back(req["category"].get<std::string>());
        }
        if (req.contains("storeAddress"))
        {
            storeClauses += "store_address=?, ";
            storeValues.push_back(req["storeAddress"].get<std::string>());
        }
        if (req.contains("cookTime"))
        {
            storeClauses += "cook_time=?, ";
            // DB에 int로 되어 있다면 std::to_string()을 씁니다. 만약 string이라면 그냥 get<std::string>()
            storeValues.push_back(req["cookTime"].get<std::string>());
        }
        if (req.contains("minOrderAmount"))
        {
            storeClauses += "min_order_amount=?, ";
            storeValues.push_back(std::to_string(req["minOrderAmount"].get<int>()));
        }
        if (req.contains("openTime"))
        {
            storeClauses += "open_time=?, ";
            storeValues.push_back(req["openTime"].get<std::string>());
        }
        if (req.contains("closeTime"))
        {
            storeClauses += "close_time=?, ";
            storeValues.push_back(req["closeTime"].get<std::string>());
        }

        // ── 3단계: USERS 테이블 변경 필드 조립 ────────────────
        std::string userClauses;
        std::vector<std::string> userValues;

        if (req.contains("ownerName"))
        {
            userClauses += "user_name=?, ";
            userValues.push_back(req["ownerName"].get<std::string>());
        }
        if (req.contains("ownerPhone"))
        {
            userClauses += "phone_number=?, ";
            userValues.push_back(req["ownerPhone"].get<std::string>());
        }

        // ── 4단계: OWNERS 테이블 변경 필드 조립 ───────────────
        std::string ownerClauses;
        std::vector<std::string> ownerValues;

        if (req.contains("accountNumber"))
        {
            ownerClauses += "account_number=?, ";
            ownerValues.push_back(req["accountNumber"].get<std::string>());
        }

        // ── 5단계: 셋 다 비어있으면 변경사항 없음 ─────────────
        if (storeClauses.empty() && userClauses.empty() && ownerClauses.empty())
        {
            json res;
            res["status"] = 400;
            res["message"] = "변경된 항목이 없습니다.";
            session->sendPacket(static_cast<int>(CmdID::RES_STORE_INFO_UPDATE), res);
            return;
        }

        // ── 6단계: STORES UPDATE 실행 ─────────────────────────
        if (success && !storeClauses.empty())
        {
            storeClauses = storeClauses.substr(0, storeClauses.size() - 2); // 마지막 ", " 제거
            std::string query = "UPDATE STORES SET " + storeClauses + " WHERE store_id=?";
            storeValues.push_back(std::to_string(storeId));

            // 싱글톤 패턴으로 구현된 DAO 호출
            success = StoreDAO::getInstance().executeUpdate(query, storeValues);
        }

        // ── 7단계: USERS UPDATE 실행 ──────────────────────────
        if (success && !userClauses.empty())
        {
            userClauses = userClauses.substr(0, userClauses.size() - 2);
            std::string query = "UPDATE USERS SET " + userClauses + " WHERE user_id=?";
            userValues.push_back(session->getUserId());
            success = UserDAO::getInstance().executeUpdate(query, userValues);
        }

        // ── 8단계: OWNERS UPDATE 실행 ─────────────────────────
        if (success && !ownerClauses.empty())
        {
            ownerClauses = ownerClauses.substr(0, ownerClauses.size() - 2);
            std::string query = "UPDATE OWNERS SET " + ownerClauses + " WHERE user_id=?";
            ownerValues.push_back(session->getUserId());
            success = UserDAO::getInstance().executeUpdate(query, ownerValues);
        }

        // ── 9단계: 응답 전송 ────────────────────────────────────
        json res;
        res["status"] = success ? 200 : 500;
        res["message"] = success ? "저장 완료" : "DB 오류가 발생했습니다.";
        session->sendPacket(static_cast<int>(CmdID::RES_STORE_INFO_UPDATE), res);
    }
    catch (const std::exception &e)
    {
        std::cerr << "[StoreHandler] 매장 정보 업데이트 에러: " << e.what() << std::endl;
        json res;
        res["status"] = 500;
        res["message"] = "서버 내부 오류";
        session->sendPacket(static_cast<int>(CmdID::RES_STORE_INFO_UPDATE), res);
    }
}

void StoreHandler::handleStoreStatusSet(std::shared_ptr<ClientSession> session,
                                        const std::string &jsonBody)
{
    nlohmann::json req = nlohmann::json::parse(jsonBody);
    nlohmann::json res;

    try
    {
        // 1. storeId, status 파싱
        int storeId = req.value("storeId", 0);
        int status = req.value("status", -1);

        if (storeId == 0 || status == -1)
        {
            res["status"] = 400;
            res["message"] = "storeId 또는 status가 없습니다.";
            session->sendPacket(static_cast<uint16_t>(CmdID::RES_STORE_STATUS_SET), res);
            return;
        }

        // 2. DB UPDATE 실행
        // UPDATE STORES SET status = ? WHERE store_id = ?
        std::string query = "UPDATE STORES SET status=? WHERE store_id=?";
        std::vector<std::string> params = {
            std::to_string(status),
            std::to_string(storeId)};

        bool success = UserDAO::getInstance().executeUpdate(query, params);

        // 3. 응답 전송
        res["status"] = success ? 200 : 500;
        res["message"] = success ? "영업 상태가 변경되었습니다." : "DB 오류가 발생했습니다.";

        std::cout << "[StoreHandler] 영업상태 변경 - storeId: " << storeId
                  << ", status: " << status
                  << ", 결과: " << (success ? "성공" : "실패") << std::endl;

        session->sendPacket(static_cast<uint16_t>(CmdID::RES_STORE_STATUS_SET), res);
    }
    catch (const std::exception &e)
    {
        std::cerr << "[StoreHandler] 오류: " << e.what() << std::endl;
        res["status"] = 500;
        res["message"] = "서버 내부 오류";
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_STORE_STATUS_SET), res);
    }
}