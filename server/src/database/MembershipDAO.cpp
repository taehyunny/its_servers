#include "MembershipDAO.h"
#include "DbManager.h"
#include <iostream>
#include <mariadb/conncpp.hpp>

bool MembershipDAO::updateMembershipGrade(const std::string &userId, const std::string &targetGrade, int action)
{
    auto conn = DBManager::getInstance().getConnection();
    try
    {
        // 🚀 1. 트랜잭션 시작 (테이블 2개도 동시에 바뀌어야 하므로 트랜잭션은 유지!)
        conn->setAutoCommit(false);

        // action이 1(구독)이면 targetGrade("와우"), 2(해지)면 "일반"으로 설정
        std::string gradeToSet = (action == 1) ? targetGrade : "일반";

        // 🚀 2. CUSTOMERS 테이블 업데이트 (오타 수정: custumers -> CUSTOMERS)
        std::unique_ptr<sql::PreparedStatement> pstmt1(conn->prepareStatement(
            "UPDATE CUSTOMERS SET customer_grade = ? WHERE user_id = ?"));
        pstmt1->setString(1, gradeToSet);
        pstmt1->setString(2, userId);
        pstmt1->executeUpdate();

        // 🚀 3. USERS 테이블 업데이트
        std::unique_ptr<sql::PreparedStatement> pstmtUsers(conn->prepareStatement(
            "UPDATE USERS SET grade = ? WHERE user_id = ?"));
        pstmtUsers->setString(1, gradeToSet);
        pstmtUsers->setString(2, userId);
        pstmtUsers->executeUpdate();

        // 🚀 4. 두 테이블 모두 에러 없이 통과했다면 영구 반영(Commit)!
        conn->commit();
        conn->setAutoCommit(true);

        std::string actionStr = (action == 1) ? "구독" : "해지";
        std::cout << "[MembershipDAO] 💎 " << userId << " 님의 멤버십 " << actionStr << " 처리 완료! (USERS & CUSTOMERS 동기화)" << std::endl;
        
        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [MembershipDAO] 등급 변경 트랜잭션 실패: " << e.what() << std::endl;
        if (conn)
        {
            conn->rollback(); // 하나라도 실패하면 둘 다 이전 상태로 되돌림!
            conn->setAutoCommit(true);
        }
        return false;
    }
}