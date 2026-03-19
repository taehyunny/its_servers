#include "UserHandler.h"
#include "AllDTOs.h"       // 1. DTO 정의(및 매크로)를 먼저 읽어야 함
#include "ClientSession.h" // 2. 그 다음 세션(템플릿 사용처)을 읽어야 함
#include "UserDAO.h"
#include "AuthDAO.h"
#include <iostream>

void UserHandler::handleSignup(ClientSession *session, const std::string &jsonBody)
{
    try
    {
        // 1. 역직렬화 (DTO로 변환)
        auto req = nlohmann::json::parse(jsonBody).get<SignupReqDTO>();

        // 2. DB 로직 (UserDAO 호출)
        bool success = UserDAO::getInstance().insertUser(req);

        // 3. 응답 DTO 생성
        AuthResDTO res;
        res.status = success ? 200 : 409;
        res.message = success ? "이츠 배달료에 오신 것을 환영합니다!" : "이미 가입된 아이디입니다.";
        res.userName = req.userName;

        // 4. 전송 (Session의 송신 로직 호출)
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_SIGNUP), res);
        std::cout << "[UserHandler] 회원가입 처리 완료: " << req.userId << " (Status: " << res.status << ")" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "[UserHandler] Signup Error: " << e.what() << std::endl;
    }
}

void UserHandler::handleLogin(ClientSession *session, const std::string &jsonBody)
{
    try
    {
        auto req = nlohmann::json::parse(jsonBody).get<LoginReqDTO>();

        // DB 인증 로직 (가상)
        auto userJson = AuthDAO::getInstance().validateLogin(req.userId, req.password);

        AuthResDTO res;
        if (!userJson.empty())
        {
            res.status = 200;
            res.userName = userJson["user_name"].get<std::string>();
            res.message = "로그인 성공";

            // 세션에 로그인 정보 박제 (핵심!)
            session->authenticate(req.userId, userJson["role"].get<int>());
        }
        else
        {
            res.status = 401;
            res.message = "아이디 또는 비밀번호가 틀렸습니다.";
        }

        session->sendPacket(static_cast<uint16_t>(CmdID::RES_LOGIN), res);
    }
    catch (const std::exception &e)
    {
        std::cerr << "[UserHandler] Login Error: " << e.what() << std::endl;
    }
}