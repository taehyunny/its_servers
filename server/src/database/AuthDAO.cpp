#include "AuthDAO.h"
#include "DbManager.h"
#include "Global_protocol.h"
#include <iostream>
#include <mariadb/conncpp.hpp>

std::pair<LoginResult, nlohmann::json> AuthDAO::validateLogin(const std::string &inputId, const std::string &inputPw)
{
    std::shared_ptr<sql::Connection> conn;
    nlohmann::json userInfo;

    try
    {
        conn = DBManager::getInstance().getConnection();

        // 1. 아이디 존재 여부 및 유저 정보 조회
        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
            "SELECT password_hash, user_name, role FROM USERS WHERE user_id = ?"));
        pstmt->setString(1, inputId);

        std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());

        // 결과가 없다면 아이디가 틀린(없는) 것
        if (!rs->next())
        {
            std::cout << "[AuthDAO] 로그인 실패: 존재하지 않는 아이디 (" << inputId << ")" << std::endl;
            return {LoginResult::ID_NOT_FOUND, userInfo};
        }

        // 2. 비밀번호 검증
        std::string dbPassword = rs->getString("password_hash").c_str();
        if (dbPassword != inputPw)
        {
            std::cout << "[AuthDAO] 로그인 실패: 비밀번호 불일치 (" << inputId << ")" << std::endl;
            return {LoginResult::WRONG_PASSWORD, userInfo};
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