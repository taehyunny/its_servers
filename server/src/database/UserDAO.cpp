#include "UserDAO.h"
#include "DbManager.h" // ConnectionPool을 관리하는 클래스라고 가정
#include <iostream>

bool UserDAO::insertUser(const SignupReqDTO &dto)
{
    try
    {
        // 1. 커넥션 풀에서 연결 하나 빌려오기
        auto conn = DBManager::getInstance().getConnection();

        // 2. PreparedStatement 준비 (SQL 인젝션 방지)
        // USERS(user_id, password_hash, user_name, phone_number, role)
        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
            "INSERT INTO USERS (user_id, password_hash, user_name, phone_number, role) VALUES (?, ?, ?, ?, ?)"));

        pstmt->setString(1, dto.userId);
        pstmt->setString(2, dto.password); // 실무에선 여기서 해싱 적용!
        pstmt->setString(3, dto.userName);
        pstmt->setString(4, dto.phoneNumber);
        pstmt->setInt(5, dto.role);

        // 3. 실행
        pstmt->executeUpdate();
        return true;
    }
    catch (sql::SQLException &e)
    {
        // 409 Conflict 발생 사유: 주로 PK 중복(Duplicate Entry)
        std::cerr << "[UserDAO] Insert Error: " << e.what() << std::endl;
        return false;
    }
}