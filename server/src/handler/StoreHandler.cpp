#include "StoreHandler.h"
#include "StoreDAO.h"
#include "UserDAO.h"
#include "AllDTOs.h"
#include "ClientSession.h"
#include "Global_protocol.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <vector>

using nlohmann::json;

// ── 1. 매장 목록 조회 ──────────────────────────────────────────
// ── 1. 매장 목록 조회 ──────────────────────────────────────────
void StoreHandler::handleStoreListRequest(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        // 🚀 1. json 파싱 대신, 새로 업데이트한 DTO를 사용해 brandName까지 쏙 뽑아옵니다!
        auto req = json::parse(jsonBody).get<StoreListReqDTO>();
        int categoryId = req.categoryId;
        std::string brandName = req.brandName; // 👈 프론트가 보낸 브랜드명!

        std::cout << "[StoreHandler] 상점 목록 요청 수신 - 카테고리 ID: " << categoryId
                  << " / 브랜드명: " << (brandName.empty() ? "없음" : brandName) << std::endl;

        StoreListResDTO resDto;
        resDto.status = 200;

        if (categoryId == 0 && brandName.empty())
        {
            // 카테고리도 0이고 브랜드명도 없으면 진짜 전체 매장
            resDto.stores = StoreDAO::getInstance().getAllStores();
        }
        else
        {
            // 🚀 2. DAO에게 categoryId와 brandName을 "둘 다" 넘겨줍니다!
            // (함수 이름도 getStoresByCategory 로 통일하는 게 좋습니다)
            resDto.stores = StoreDAO::getInstance().getStoresByCategory(categoryId, brandName);
        }

        nlohmann::json resJson = resDto;
        std::cout << ">>> [DEBUG] 전송할 JSON 데이터:\n"
                  << resJson.dump(4) << std::endl;

        session->sendPacket(static_cast<uint16_t>(CmdID::RES_STORE_LIST), resJson);
        std::cout << "[StoreHandler] 전송 완료." << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [StoreHandler] 에러 발생: " << e.what() << std::endl;
        StoreListResDTO errorDto;
        errorDto.status = 500;
        nlohmann::json errorJson = errorDto;
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_STORE_LIST), errorJson);
    }
}

// ── 2. 매장 상세 조회 (2002번) ──────────────────────────────────
void StoreHandler::handleStoreDetailReq(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        // 🚀 1. 새로 만든 Req DTO를 사용하여 안전하게 파싱
        auto req = nlohmann::json::parse(jsonBody).get<ReqStoreDetailDTO>();
        int storeId = req.storeId;

        if (storeId == 0)
        {
            json res;
            res["status"] = 400;
            res["message"] = "storeId가 누락되었습니다.";
            session->sendPacket(static_cast<uint16_t>(CmdID::RES_STORE_DETAIL), res);
            return;
        }

        // 🚀 2. DAO에서 '종합 선물 세트(ResStoreDetailDTO)'를 가져옴
        ResStoreDetailDTO storeDetailRes = StoreDAO::getInstance().getStoreDetail(storeId);

        // 🚀 3. JSON으로 변환하여 전송 (ResStoreDetailDTO에 to_json이 필요할 수 있음)
        nlohmann::json resJson = storeDetailRes;

        std::cout << "[StoreHandler] 매장 상세 조회 완료 - storeId: " << storeId << std::endl;
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_STORE_DETAIL), resJson);
    }
    catch (const std::exception &e)
    {
        std::cerr << "[StoreHandler] 매장 상세 조회 에러: " << e.what() << std::endl;
        json res;
        res["status"] = 500;
        res["message"] = "서버 내부 오류";
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_STORE_DETAIL), res);
    }
}

// ── 3. 매장 정보 수정 (사장님 전용) ──────────────────────────────────
void StoreHandler::handleStoreInfoUpdateReq(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        // 🚀 수정: DTO로 바꾸지 말고, 날것의 JSON 객체로 파싱합니다.
        json req = json::parse(jsonBody);

        // 🚀 수정: JSON에서 값을 안전하게 꺼내옵니다.
        int storeId = req.value("storeId", 0);

        if (storeId == 0)
        {
            json res;
            res["status"] = 400;
            res["message"] = "storeId가 없습니다.";
            session->sendPacket(static_cast<uint16_t>(CmdID::RES_STORE_INFO_UPDATE), res);
            return;
        }

        bool success = true;
        std::string storeClauses;
        std::vector<std::string> storeValues;

        // 🚀 이제 req가 json 객체이므로 contains와 [] 연산자가 완벽하게 동작합니다!
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
            storeValues.push_back(req["cookTime"].get<std::string>());
        }
        if (req.contains("deliveryFee"))
        {
            storeClauses += "delivery_fee=?, ";
            storeValues.push_back(std::to_string(req["deliveryFee"].get<int>()));
        }

        // 🚀 방어 로직: 숫자로 오든, 문자로 오든 안전하게 문자열로 변환하여 쿼리에 바인딩
        if (req.contains("minOrderAmount"))
        {
            storeClauses += "min_order_amount=?, ";
            if (req["minOrderAmount"].is_number())
            {
                storeValues.push_back(std::to_string(req["minOrderAmount"].get<int>()));
            }
            else
            {
                storeValues.push_back(req["minOrderAmount"].get<std::string>());
            }
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

        std::string ownerClauses;
        std::vector<std::string> ownerValues;

        if (req.contains("accountNumber"))
        {
            ownerClauses += "account_number=?, ";
            ownerValues.push_back(req["accountNumber"].get<std::string>());
        }

        if (storeClauses.empty() && userClauses.empty() && ownerClauses.empty())
        {
            json res;
            res["status"] = 400;
            res["message"] = "변경된 항목이 없습니다.";
            // 🚀 수정: int -> uint16_t
            session->sendPacket(static_cast<uint16_t>(CmdID::RES_STORE_INFO_UPDATE), res);
            return;
        }

        if (success && !storeClauses.empty())
        {
            storeClauses = storeClauses.substr(0, storeClauses.size() - 2);
            std::string query = "UPDATE STORES SET " + storeClauses + " WHERE store_id=?";
            storeValues.push_back(std::to_string(storeId));
            success = StoreDAO::getInstance().executeUpdate(query, storeValues);
        }

        if (success && !userClauses.empty())
        {
            userClauses = userClauses.substr(0, userClauses.size() - 2);
            std::string query = "UPDATE USERS SET " + userClauses + " WHERE user_id=?";
            userValues.push_back(session->getUserId());
            success = UserDAO::getInstance().executeUpdate(query, userValues);
        }

        if (success && !ownerClauses.empty())
        {
            ownerClauses = ownerClauses.substr(0, ownerClauses.size() - 2);
            std::string query = "UPDATE OWNERS SET " + ownerClauses + " WHERE user_id=?";
            ownerValues.push_back(session->getUserId());
            success = UserDAO::getInstance().executeUpdate(query, ownerValues);
        }

        json res;
        res["status"] = success ? 200 : 500;
        res["message"] = success ? "저장 완료" : "DB 오류가 발생했습니다.";
        // 🚀 수정: int -> uint16_t
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_STORE_INFO_UPDATE), res);
    }
    catch (const std::exception &e)
    {
        std::cerr << "[StoreHandler] 매장 정보 업데이트 에러: " << e.what() << std::endl;
        json res;
        res["status"] = 500;
        res["message"] = "서버 내부 오류";
        // 🚀 수정: int -> uint16_t
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_STORE_INFO_UPDATE), res);
    }
}

// ── 4. 영업 상태 변경 (사장님 전용) ──────────────────────────────────
void StoreHandler::handleStoreStatusSet(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    // ... 기존 코드와 동일 (수정할 부분 없음) ...
    try
    {
        json req = json::parse(jsonBody);
        json res;

        int storeId = req.value("storeId", 0);
        int status = req.value("status", -1);

        if (storeId == 0 || status == -1)
        {
            res["status"] = 400;
            res["message"] = "storeId 또는 status가 없습니다.";
            session->sendPacket(static_cast<uint16_t>(CmdID::RES_STORE_STATUS_SET), res);
            return;
        }

        std::string query = "UPDATE STORES SET status=? WHERE store_id=?";
        std::vector<std::string> params = {
            std::to_string(status),
            std::to_string(storeId)};

        bool success = StoreDAO::getInstance().executeUpdate(query, params);

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
        json res;
        res["status"] = 500;
        res["message"] = "서버 내부 오류";
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_STORE_STATUS_SET), res);
    }
}