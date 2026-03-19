#include "AuthDAO.h"
#include "DbManager.h" // 서버용 MariaDB 커넥션 풀 매니저
#include <iostream>

nlohmann::json AuthDAO::validateLogin(const std::string &userId, const std::string &password)
{
    nlohmann::json userDoc;

    try
    {
        // 서버용 DBManager에서 연결 빌려오기 (DSDBManager의 connectDatabase 역할)
        auto conn = DBManager::getInstance().getConnection();

        // 쿼리 준비 (PreparedStatement 사용으로 보안 강화)
        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
            "SELECT user_name, role FROM USERS WHERE user_id = ? AND password_hash = ?"));

        pstmt->setString(1, userId);
        pstmt->setString(2, password); // 실제로는 해싱된 값과 비교

        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

        if (res->next())
        {
            // 로그인 성공 시 정보를 JSON으로 담기 (태현님의 JSON 선호 스타일 반영)
            userDoc["user_name"] = std::string(res->getString("user_name"));
            userDoc["role"] = res->getInt("role");
        }
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "❌ AuthDAO 로그인 검증 실패: " << e.what() << std::endl;
    }

    return userDoc;
}