#include "MembershipDAO.h"
#include "DbManager.h"
#include <iostream>
#include <mariadb/conncpp.hpp>

bool MembershipDAO::updateMembershipGrade(const std::string &userId, const std::string &targetGrade, int action)
{
    auto conn = DBManager::getInstance().getConnection();
    try
    {
        // 1. 트랜잭션 시작 (둘 다 성공해야만 인정!)
        conn->setAutoCommit(false);

        std::unique_ptr<sql::PreparedStatement> pstmtUser(conn->prepareStatement(
            "UPDATE USERS SET grade = ? WHERE user_id = ?"));
        pstmtUser->setString(1, targetGrade);
        pstmtUser->setString(2, userId);
        pstmtUser->executeUpdate();

        std::unique_ptr<sql::PreparedStatement> pstmtCustomer(conn->prepareStatement(
            "UPDATE CUSTOMERS SET customer_grade = ? WHERE user_id = ?"));
        pstmtCustomer->setString(1, targetGrade);
        pstmtCustomer->setString(2, userId);
        pstmtCustomer->executeUpdate();

        conn->commit();
        conn->setAutoCommit(true);

        std::cout << "[UserDAO] 💎 " << userId << " 님의 등급이 '" << targetGrade << "'(으)로 완벽하게 동기화되었습니다!" << std::endl;
        return true;
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "🚨 [UserDAO] 등급 업데이트 트랜잭션 실패: " << e.what() << std::endl;
        if (conn)
        {
            // 하나라도 오타나 에러가 나면 롤백해서 데이터 꼬임을 방지합니다.
            conn->rollback();
            conn->setAutoCommit(true);
        }
        return false;
    }
}