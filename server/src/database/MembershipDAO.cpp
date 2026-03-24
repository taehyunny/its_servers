#include "MembershipDAO.h"
#include "DbManager.h"
#include <iostream>
#include <mariadb/conncpp.hpp> // 🚀 모든 상세 명세가 들어있는 헤더

bool MembershipDAO::changeUserGrade(const std::string &userId, const std::string &newGrade)
{
    auto conn = DBManager::getInstance().getConnection();
    if (!conn)
        return false;

    try
    {
        conn->setAutoCommit(false);

        // ✅ 수정된 부분: 대입(=)이 아닌 생성자() 방식을 사용합니다.
        std::unique_ptr<sql::PreparedStatement> pstmtSync(
            conn->prepareStatement("UPDATE CUSTOMERS SET grade_name = ? WHERE user_id = ?"));

        pstmtSync->setString(1, newGrade);
        pstmtSync->setString(2, userId);
        pstmtSync->executeUpdate();

        conn->commit();
        conn->setAutoCommit(true);
        return true;
    }
    catch (sql::SQLException &e)
    {
        if (conn)
        {
            conn->rollback();
            conn->setAutoCommit(true);
        }
        std::cerr << "🚨 [MembershipDAO] DB 에러: " << e.what() << std::endl;
        return false;
    }
}