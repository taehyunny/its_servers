#include "UserHandler.h"
#include "AllDTOs.h"       // 1. DTO 정의(및 매크로)를 먼저 읽어야 함
#include "ClientSession.h" // 2. 그 다음 세션(템플릿 사용처)을 읽어야 함
#include "SessionManager.h"
#include "UserDAO.h"
#include "AuthDAO.h"
#include "MembershipDAO.h"
#include "DbManager.h"
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

        // 1. AuthDAO 호출 (ID, PW, Role 검증)
        auto [resultCode, userJson] = AuthDAO::getInstance().validateLogin(req.userId, req.password, req.role);

        if (resultCode == LoginResult::SUCCESS)
        {
            AuthResDTO res;
            res.status = 200;
            res.message = "로그인 성공";
            res.userId = userJson.value("userId", "");
            res.userName = userJson.value("userName", "");
            res.address = userJson.value("address", "");
            res.phoneNumber = userJson.value("phoneNumber", "");

            // 🚀 [Fact Check] DB에서 꺼내온 grade를 응답 DTO에 꽂아줍니다!
            res.grade = userJson.value("grade", "일반");

            std::string roleStr = userJson.value("role", "0");
            int dbRole = std::stoi(roleStr);
            res.role = roleStr;

            // 2. 사장님(role == 1) 전용 데이터 매핑
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

            // 🚀 [핵심 로직] 세션 등록 (이게 있어야 다른 API 통신이 안 끊깁니다!)
            session->authenticate(res.userId, dbRole);
            SessionManager::getInstance().registerUser(res.userId, dbRole, session);

            std::cout << "[UserHandler] 로그인 성공! 유저 '" << res.userId << "' [" << res.grade << "] 등급 세션 등록 완료." << std::endl;

            // 🚀 3. 다이어트된 순수한 DTO 전송 (여기에 brandCategories는 없습니다!)
            session->sendPacket(static_cast<uint16_t>(CmdID::RES_LOGIN), res);
        }
        else
        {
            // ❌ 실패 시 빈 res 객체가 아니라, 에러 메시지를 담은 객체를 보냅니다.
            AuthResDTO errRes;
            if (resultCode == LoginResult::ID_PASS_WRONG)
            {
                errRes.status = 401;
                errRes.message = "아이디/비밀번호가 틀렸거나 권한이 맞지 않습니다.";
            }
            else
            {
                errRes.status = 500;
                errRes.message = "로그인 처리 중 서버 오류가 발생했습니다.";
            }
            session->sendPacket(static_cast<uint16_t>(CmdID::RES_LOGIN), errRes);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [UserHandler] Login Error: " << e.what() << std::endl;
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
    auto conn = DBManager::getInstance().getConnection();
    try
    {
        // 1. DTO 파싱
        auto req = nlohmann::json::parse(jsonBody).get<ReqGradeUpdateDTO>();

        // 🚀 디버깅용: 파싱이 제대로 됐는지 확인!
        std::cout << "[UserHandler] 파싱된 데이터 - UserID: [" << req.userId << "], Grade: [" << req.grade << "]" << std::endl;

        // 💡 만약 req.userId가 비어있다면, JSON 키 대소문자가 안 맞는 것입니다!
        if (req.userId.empty())
        {
            throw std::runtime_error("UserId가 비어있습니다. JSON 키값을 확인하세요.");
        }

        std::string displayMsg = "";
        std::string targetGrade = (req.action == 1) ? "와우" : "일반"; // 클라이언트가 "wow"로 보내도 DB엔 "와우"로 강제 세팅

        if (req.action == 1)
        {
            displayMsg = "멤버십 가입이 완료되었습니다! 이제 배달비가 0원입니다. 🎉";
        }
        else if (req.action == 2)
        {
            displayMsg = "멤버십 해지가 예약되었습니다. 이용 기간 종료 후 일반 등급으로 변경됩니다.";
        }

        // 🚀 2. 트랜잭션 시작 (USERS와 CUSTOMERS 둘 다 동기화!)
        conn->setAutoCommit(false);

        // [2-A] USERS 테이블 업데이트
        std::unique_ptr<sql::PreparedStatement> pstmtUser(conn->prepareStatement(
            "UPDATE USERS SET grade = ? WHERE user_id = ?"));
        pstmtUser->setString(1, targetGrade);
        pstmtUser->setString(2, req.userId);
        pstmtUser->executeUpdate();

        // [2-B] CUSTOMERS 테이블 업데이트 (결제 혜택 연동!)
        std::unique_ptr<sql::PreparedStatement> pstmtCustomer(conn->prepareStatement(
            "UPDATE CUSTOMERS SET customer_grade = ? WHERE user_id = ?"));
        pstmtCustomer->setString(1, targetGrade);
        pstmtCustomer->setString(2, req.userId);
        pstmtCustomer->executeUpdate();

        // 모두 성공하면 영구 반영
        conn->commit();
        conn->setAutoCommit(true);

        // 3. 성공 응답 전송
        ResGradeUpdateDTO res;
        res.status = 200;
        res.message = displayMsg;

        session->sendPacket(static_cast<uint16_t>(CmdID::RES_GRADE_UPDATE), nlohmann::json(res));

        std::cout << "[UserHandler] ✅ 등급 변경 완료: " << req.userId << " -> " << targetGrade << " (Action: " << req.action << ")" << std::endl;
    }
    catch (const std::exception &e)
    {
        if (conn)
        {
            conn->rollback();
            conn->setAutoCommit(true);
        }
        std::cerr << "🚨 [UserHandler] 등급 업데이트 에러: " << e.what() << std::endl;
        ResGradeUpdateDTO res = {500, "서버 오류로 등급 변경에 실패했습니다."};
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_GRADE_UPDATE), nlohmann::json(res));
    }
}