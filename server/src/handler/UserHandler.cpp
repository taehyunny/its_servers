#include "UserHandler.h"
#include "AllDTOs.h"       // 1. DTO 정의(및 매크로)를 먼저 읽어야 함
#include "ClientSession.h" // 2. 그 다음 세션(템플릿 사용처)을 읽어야 함
#include "UserDAO.h"
#include "AuthDAO.h"
#include <iostream>

// 🚀 [수정] ClientSession* -> std::shared_ptr<ClientSession> 으로 변경
void UserHandler::handleSignup(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    AuthResDTO res;
    try
    {
        auto req = nlohmann::json::parse(jsonBody).get<SignupReqDTO>();
        SignupResult result = UserDAO::getInstance().insertUser(req);

        if (result == SignupResult::SUCCESS)
        {
            res.status = 200;
            res.message = "회원가입 성공";
            res.userName = req.userName;
        }
        else if (result == SignupResult::DUPLICATE_ID)
        {
            res.status = 409;
            res.message = "이미 사용 중인 아이디입니다.";
        }
        else if (result == SignupResult::DUPLICATE_PHONE)
        {
            res.status = 410; // 또는 409
            res.message = "이미 등록된 전화번호입니다.";
        }
        else
        {
            res.status = 500;
            res.message = "서버 내부 오류 (DB 혹은 데이터 규격 확인 필요)";
            std::cout << ">>> [ERROR] 회원가입 실패 결과 코드: " << static_cast<int>(result) << std::endl;
        }
    }
    catch (const std::exception &e)
    {
        res.status = 400;
        res.message = "데이터 형식이 잘못되었습니다.";
        std::cerr << ">>> [JSON ERROR] " << e.what() << std::endl;
    }

    session->sendPacket(static_cast<uint16_t>(CmdID::RES_SIGNUP), res);
}

// 🚀 [수정] ClientSession* -> std::shared_ptr<ClientSession> 으로 변경
void UserHandler::handleLogin(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        auto req = nlohmann::json::parse(jsonBody).get<LoginReqDTO>();
        auto [resultCode, userJson] = UserDAO::getInstance().checkLogin(req.userId, req.password);

        AuthResDTO res;

        if (resultCode == LoginResult::SUCCESS)
        {
            res.status = 200;
            res.message = "로그인 성공";
            res.userId = userJson["user_id"].get<std::string>();
            res.userName = userJson["user_name"].get<std::string>();
            res.address = userJson.value("address", "");
            res.phoneNumber = userJson.value("phone_number", "");
            res.role = std::to_string(userJson.value("role", 0));
            res.storeName = userJson.value("store_name", "");

            session->authenticate(res.userId, userJson["role"].get<int>());
        }
        else if (resultCode == LoginResult::ID_PASS_WRONG)
        {
            res.status = 401;
            res.message = "아이디 비밀번호가 틀렸습니다.";
        }
        else
        {
            res.status = 500;
            res.message = "로그인 처리 중 서버 오류가 발생했습니다.";
        }

        // 🚀 [핵심] 모든 결과 처리가 끝난 후, 여기서 "딱 한 번만" 쏩니다!
        std::cout << ">>> [DEBUG] 클라이언트로 쏘는 최종 JSON: " << nlohmann::json(res).dump(4) << std::endl;
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_LOGIN), res);
    }
    catch (const std::exception &e)
    {
        std::cerr << "[UserHandler] Login Error: " << e.what() << std::endl;
        // 필요하다면 여기서도 에러 패킷을 쏠 수 있습니다.
    }
}

// 🚀 [수정] ClientSession* -> std::shared_ptr<ClientSession> 으로 변경
void UserHandler::handleAuthCheck(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    AuthCheckResDTO res;
    try
    {
        auto req = nlohmann::json::parse(jsonBody).get<AuthCheckReqDTO>();
        bool exists = UserDAO::getInstance().existsById(req.userId);

        if (exists)
        {
            res.status = 409;
            res.isAvailable = false;
            res.message = "이미 사용 중인 아이디입니다.";
        }
        else
        {
            res.status = 200;
            res.isAvailable = true;
            res.message = "사용 가능한 아이디입니다.";
        }
    }
    catch (...)
    {
        res.status = 400;
        res.isAvailable = false;
        res.message = "잘못된 요청입니다.";
    }
    session->sendPacket(static_cast<uint16_t>(CmdID::RES_AUTH_CHECK), res);
}

// 🚀 [수정] ClientSession* -> std::shared_ptr<ClientSession> 으로 변경
void UserHandler::handlePhoneCheck(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    PhoneCheckResDTO res;
    try
    {
        auto req = nlohmann::json::parse(jsonBody).get<PhoneCheckReqDTO>();
        bool exists = UserDAO::getInstance().existsByPhoneAndRole(req.phoneNumber, req.role);

        if (exists)
        {
            res.status = 409;
            res.isAvailable = false;
            res.message = "해당 역할로 이미 가입된 번호입니다.";
        }
        else
        {
            res.status = 200;
            res.isAvailable = true;
            res.message = "사용 가능한 번호입니다.";
        }
    }
    catch (...)
    {
        res.status = 400;
        res.isAvailable = false;
        res.message = "잘못된 요청입니다.";
    }
    session->sendPacket(static_cast<uint16_t>(CmdID::RES_PHONE_CHECK), res);
}

// 이 함수는 이미 잘 되어 있었습니다!
void UserHandler::handleBizNumCheck(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    BizNumCheckResDTO res;
    try
    {
        auto req = nlohmann::json::parse(jsonBody).get<BizNumCheckReqDTO>();
        bool exists = UserDAO::getInstance().existsByBizNum(req.businessNum);

        if (exists)
        {
            res.status = 409;
            res.isAvailable = false;
            res.message = "이미 등록된 사업자 번호입니다.";
        }
        else
        {
            res.status = 200;
            res.isAvailable = true;
            res.message = "사용 가능한 사업자 번호입니다.";
        }
    }
    catch (...)
    {
        res.status = 400;
        res.isAvailable = false;
        res.message = "잘못된 요청 형식입니다.";
    }
    session->sendPacket(static_cast<uint16_t>(CmdID::RES_BUISNESS_NUM_CHECK), res);
}