#include "UserHandler.h"
#include "AllDTOs.h"       // 1. DTO 정의(및 매크로)를 먼저 읽어야 함
#include "ClientSession.h" // 2. 그 다음 세션(템플릿 사용처)을 읽어야 함
#include "UserDAO.h"
#include "AuthDAO.h"
#include <iostream>

void UserHandler::handleSignup(ClientSession *session, const std::string &jsonBody)
{
    AuthResDTO res; // 로그인과 동일한 응답 DTO를 재활용합니다.
    try
    {
        auto req = nlohmann::json::parse(jsonBody).get<SignupReqDTO>(); // JSON을 DTO로 변환

        // 🚀 [수정됨] int가 아니라 SignupResult 열거형으로 받습니다!
        SignupResult result = UserDAO::getInstance().insertUser(req); // 회원가입 시도 및 결과 받기

        // 🚀 [수정됨] 숫자가 아닌 명확한 Enum 값으로 비교합니다.
        if (result == SignupResult::SUCCESS)
        {
            res.status = 200;
            res.message = "회원가입 성공";
            res.userName = req.userName; // 성공 시 유저 이름도 담아줍니다.
        }
        else if (result == SignupResult::DUPLICATE_ID)
        {
            res.status = 409;
            res.message = "이미 사용 중인 아이디입니다.";
        }
        else if (result == SignupResult::DUPLICATE_PHONE)
        {
            res.status = 410;
            res.message = "이미 등록된 전화번호입니다.";
        }
        else
        {
            res.status = 500;
            res.message = "서버 내부 오류가 발생했습니다.";
        }
    }
    catch (...)
    {
        res.status = 400;
        res.message = "데이터 형식이 잘못되었습니다.";
    }

    session->sendPacket(static_cast<uint16_t>(CmdID::RES_SIGNUP), res);
}

void UserHandler::handleLogin(ClientSession *session, const std::string &jsonBody)
{
    std::cout << ">>> [DEBUG] 수신된 JSON 원본: " << jsonBody << std::endl;
    try
    {
        auto req = nlohmann::json::parse(jsonBody).get<LoginReqDTO>(); // JSON을 DTO로 변환

        // 🚀 [수정됨] std::pair 상자를 열어서 resultCode와 userJson으로 각각 받습니다!
        auto [resultCode, userJson] = AuthDAO::getInstance().validateLogin(req.userId, req.password);

        // 이제 userJson은 순수한 JSON 객체이므로 dump()를 쓸 수 있습니다.
        std::cout << ">>> [DEBUG] DB 인증 결과: " << userJson.dump() << std::endl;

        AuthResDTO res;

        // 🚀 [수정됨] Enum 값을 통해 아주 디테일하게 응답을 쪼개줍니다.
        if (resultCode == LoginResult::SUCCESS)
        {
            res.status = 200;
            res.message = "로그인 성공";
            res.userName = userJson["user_name"].get<std::string>();

            // 세션에 로그인 정보 박제
            session->authenticate(req.userId, userJson["role"].get<int>());
        }
        else if (resultCode == LoginResult::ID_NOT_FOUND)
        {
            res.status = 404; // 아이디 없음
            res.message = "존재하지 않는 아이디입니다.";
        }
        else if (resultCode == LoginResult::WRONG_PASSWORD)
        {
            res.status = 401; // 비밀번호 틀림
            res.message = "비밀번호가 일치하지 않습니다.";
        }
        else
        {
            res.status = 500;
            res.message = "로그인 처리 중 서버 오류가 발생했습니다.";
        }

        session->sendPacket(static_cast<uint16_t>(CmdID::RES_LOGIN), res);
    }
    catch (const std::exception &e)
    {
        std::cerr << "[UserHandler] Login Error: " << e.what() << std::endl;
    }
}
void UserHandler::handleAuthCheck(ClientSession *session, const std::string &jsonBody)
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
    // 🚀 1041번으로 발사!
    session->sendPacket(static_cast<uint16_t>(CmdID::RES_AUTH_CHECK), res);
}

void UserHandler::handlePhoneCheck(ClientSession *session, const std::string &jsonBody)
{
    PhoneCheckResDTO res;
    try
    {
        auto req = nlohmann::json::parse(jsonBody).get<PhoneCheckReqDTO>();
        // 🚀 번호와 역할을 같이 넘겨서 검사!
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
    // 🚀 1043번으로 발사!
    session->sendPacket(static_cast<uint16_t>(CmdID::RES_PHONE_CHECK), res);
}