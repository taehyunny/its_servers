#include "AuthDAO.h"
#include "DbManager.h"
#include "Global_protocol.h"
#include <iostream>
#include <mariadb/conncpp.hpp>
//  AuthDAO는 로그인 검증을 담당하는 DAO입니다. DB에서 아이디와 비밀번호를 조회하여 검증 결과와 유저 정보를 반환합니다. DB 연결은 DBManager의 커넥션 풀에서 가져옵니다.
std::pair<LoginResult, nlohmann::json> AuthDAO::validateLogin(const std::string &inputId, const std::string &inputPw)
{
    std::shared_ptr<sql::Connection> conn; // 스마트 포인터 사용으로 DB 연결 관리가 훨씬 안전해졌습니다!
    nlohmann::json userInfo;

    try
    {
        conn = DBManager::getInstance().getConnection(); // DBManager에서 커넥션을 빌려옵니다. 스마트 포인터이므로 사용 후 자동으로 풀에 반환됩니다.

        // 1. 아이디 존재 여부 및 유저 정보 조회
        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
            "SELECT password_hash, user_name, role FROM USERS WHERE user_id = ?"));
        pstmt->setString(1, inputId);

        std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());

        // 결과가 없다면 아이디가 틀린(없는) 것
        if (!rs->next())
        {
            std::cout << "[AuthDAO] 로그인 실패: 아이디 혹은 비밀번호가 일치하지 않습니다. (" << inputId << ")" << std::endl;
            return {LoginResult::ID_PASS_WRONG, userInfo};
        }

        // 2. 비밀번호 검증
        std::string dbPassword = rs->getString("password_hash").c_str();
        if (dbPassword != inputPw)
        {
            std::cout << "[AuthDAO] 로그인 실패: 아이디 혹은 비밀번호가 일치하지 않습니다. (" << inputId << ")" << std::endl;
            return {LoginResult::ID_PASS_WRONG, userInfo};
        }

        // 3. 인증 성공 시 데이터 세팅
        userInfo["user_name"] = rs->getString("user_name");
        userInfo["role"] = rs->getInt("role");

        std::cout << "[AuthDAO] 로그인 성공 (" << inputId << ")" << std::endl;
        return {LoginResult::SUCCESS, userInfo};
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "[AuthDAO] DB 오류: " << e.what() << std::endl;
        return {LoginResult::SERVER_ERROR, userInfo};
    }
}