#include "UserHandler.h"
#include "AllDTOs.h"       // 1. DTO 정의(및 매크로)를 먼저 읽어야 함
#include "ClientSession.h" // 2. 그 다음 세션(템플릿 사용처)을 읽어야 함
#include "SessionManager.h"
#include "UserDAO.h"
#include "AuthDAO.h"
#include "MembershipDAO.h"
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
// 🚀 [완벽 수정본] ClientSession* -> std::shared_ptr<ClientSession>
void UserHandler::handleLogin(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        auto req = nlohmann::json::parse(jsonBody).get<LoginReqDTO>();

        // 🚀 1. UserDAO가 아니라 AuthDAO 호출! (req.role 까지 3개의 파라미터 전달)
        auto [resultCode, userJson] = AuthDAO::getInstance().validateLogin(req.userId, req.password, req.role);

        AuthResDTO res;

        if (resultCode == LoginResult::SUCCESS)
        {
            res.status = 200;
            res.message = "로그인 성공";
            res.userId = userJson.value("user_id", userJson.value("userId", ""));
            res.userName = userJson.value("userName", userJson.value("user_name", ""));
            res.address = userJson.value("address", "");
            res.phoneNumber = userJson.value("phoneNumber", "");

            std::string roleStr = userJson.value("role", "0");
            int dbRole = std::stoi(roleStr);

            res.role = roleStr;

            // 🚀 2. 사장님(role == 1)일 경우 꽉꽉 채워주는 전용 데이터 매핑!
            if (dbRole == 1)
            {
                res.businessNumber = userJson.value("businessNumber", "");
                res.accountNumber = userJson.value("accountNumber", "");
                res.approvalStatus = userJson.value("approvalStatus", 0);
                res.storeId = userJson.value("storeId", 0);
                res.storeName = userJson.value("storeName", "");
                res.category = userJson.value("category", "");
                res.storeAddress = userJson.value("storeAddress", "");
                res.cookTime = userJson.value("cookTime", "0");
                res.minOrderAmount = userJson.value("minOrderAmount", "0");
                res.openTime = userJson.value("openTime", "");
                res.closeTime = userJson.value("closeTime", "");
                res.deliveryFee = userJson.value("deliveryFee", 0);
            }

            session->authenticate(res.userId, dbRole);
            SessionManager::getInstance().authenticateUser(res.userId, session);
            std::cout << "[UserHandler] 로그인 성공! 유저 '" << res.userId << "' 세션 맵 등록 완료." << std::endl;
        }
        else if (resultCode == LoginResult::ID_PASS_WRONG)
        {
            res.status = 401;
            res.message = "아이디/비밀번호가 틀렸거나 권한이 맞지 않습니다."; // 메시지 디테일 추가
        }
        else
        {
            res.status = 500;
            res.message = "로그인 처리 중 서버 오류가 발생했습니다.";
        }

        std::cout << ">>> [DEBUG] 클라이언트로 쏘는 최종 JSON: \n"
                  << nlohmann::json(res).dump(4) << std::endl;
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_LOGIN), res);
    }
    catch (const std::exception &e)
    {
        std::cerr << "[UserHandler] Login Error: " << e.what() << std::endl;
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

void UserHandler::handleLogout(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    json res;
    try
    {
        std::string userId = session->getUserId();

        if (!userId.empty())
        {
            // 🚀 1. SessionManager의 논리적 망(userMap)에서 유저 삭제
            // (SessionManager에 removeUser 함수를 하나 만들어주면 좋습니다)
            SessionManager::getInstance().removeUser(userId);

            // 🚀 2. 현재 소켓 세션의 신원 정보 초기화 (ClientSession에 clearAuth 추가 필요)
            session->clearAuth();

            std::cout << "[UserHandler] 유저 '" << userId << "' 논리적 로그아웃 완료. (소켓 유지)" << std::endl;
        }

        // 🚀 3. 클라이언트에게 "로그아웃 성공" 응답 전송
        res["status"] = 200;
        res["message"] = "안전하게 로그아웃 되었습니다.";
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_LOGOUT), res);
    }
    catch (const std::exception &e)
    {
        res["status"] = 500;
        res["message"] = "서버 내부 오류";
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_LOGOUT), res);
    }
}

void UserHandler::handleGradeUpdate(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        json req = json::parse(jsonBody);
        std::string userId = req.value("userId", "");
        std::string targetGrade = req.value("targetGrade", "일반"); // 바꿀 등급 이름

        // DAO 호출
        bool success = MembershipDAO::getInstance().changeUserGrade(userId, targetGrade);

        json res;
        res["status"] = success ? 200 : 500;
        res["message"] = success ? "등급 변경 완료" : "등급 변경 실패";
        res["currentGrade"] = targetGrade;

        // 업그레이드면 2095, 다운그레이드면 2097로 응답 (요청에 따라 유연하게 처리 가능)
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_UPGRADE_NAME), res);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Handler Error: " << e.what() << std::endl;
    }
}