#include "UserHandler.h"
#include "AllDTOs.h"       // 1. DTO 정의(및 매크로)를 먼저 읽어야 함
#include "ClientSession.h" // 2. 그 다음 세션(템플릿 사용처)을 읽어야 함
#include "UserDAO.h"
#include "AuthDAO.h"
#include <iostream>

void UserHandler::handleSignup(ClientSession *session, const std::string &jsonBody)
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
        // 🚀 [필수 추가] 폰 번호 중복 시 500이 아닌 정확한 에러 전달!
        else if (result == SignupResult::DUPLICATE_PHONE)
        {
            res.status = 410; // 또는 409
            res.message = "이미 등록된 전화번호입니다.";
        }
        // 🚀 [디버깅 추가] 서버 에러 시 로그 출력
        else
        {
            res.status = 500;
            res.message = "서버 내부 오류 (DB 혹은 데이터 규격 확인 필요)";
            std::cout << ">>> [ERROR] 회원가입 실패 결과 코드: " << static_cast<int>(result) << std::endl;
        }
    }
    catch (const std::exception &e)
    { // catch(...) 대신 상세 에러 출력
        res.status = 400;
        res.message = "데이터 형식이 잘못되었습니다.";
        std::cerr << ">>> [JSON ERROR] " << e.what() << std::endl;
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

            // 🚀 나중에 팀원이 "아 맞다 주소!" 할 때를 대비한 '보험'들
            res.address = userJson.value("address", ""); // 없으면 빈 문자열
            res.phoneNumber = userJson.value("phone_number", "");
            res.role = std::to_string(userJson.value("role", 0));
            res.storeName = userJson.value("store_name", ""); // 사장님일 경우 대비

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

void UserHandler::handleBizNumCheck(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    BizNumCheckResDTO res;
    try
    {
        auto req = nlohmann::json::parse(jsonBody).get<BizNumCheckReqDTO>();
        // 🚀 DAO에 사업자 번호 존재 여부를 묻습니다.
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