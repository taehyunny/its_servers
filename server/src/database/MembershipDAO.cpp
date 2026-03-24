#include "MembershipDAO.h"
#include "DbManager.h"
#include "TimeUtil.h"
#include "AllDTOs.h"
#include <iostream>

// 🚀 이 헤더들이 있어야 컴파일러가 클래스 구조를 완벽히 이해합니다!
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

bool MembershipDAO::changeUserGrade(const std::string &userId, const std::string &newGrade)
{
    auto conn = DBManager::getInstance().getConnection();

    try
    {
        conn->setAutoCommit(false); // 🚀 [Step 1] 트랜잭션 시작

        // ── 1. 기존 활성화된 멤버십 비활성화 ──
        std::unique_ptr<sql::PreparedStatement> pstmtDisable;
        // 🚀 .reset()을 사용하여 명시적으로 포인터를 할당합니다. (에러 해결 핵심!)
        pstmtDisable.reset(conn->prepareStatement(
            "UPDATE USER_MEMBERSHIPS SET is_active = 0 WHERE user_id = ? AND is_active = 1"));

        pstmtDisable->setString(1, userId);
        pstmtDisable->executeUpdate();

        // ── 2. USER_MEMBERSHIPS 히스토리 추가 ──
        std::unique_ptr<sql::PreparedStatement> pstmtInsert;
        pstmtInsert.reset(conn->prepareStatement(
            "INSERT INTO USER_MEMBERSHIPS (user_id, grade_name, start_date, end_date, is_active) "
            "VALUES (?, ?, ?, ?, 1)"));

        pstmtInsert->setString(1, userId);
        pstmtInsert->setString(2, newGrade);
        pstmtInsert->setString(3, TimeUtil::getCurrentDateTime());
        pstmtInsert->setString(4, TimeUtil::getExpiryDate());
        pstmtInsert->executeUpdate();

        // ── 3. CUSTOMERS 테이블 등급 정보 동기화 ──
        std::unique_ptr<sql::PreparedStatement> pstmtSync;
        pstmtSync.reset(conn->prepareStatement(
            "UPDATE CUSTOMERS SET grade_name = ? WHERE user_id = ?"));

        pstmtSync->setString(1, newGrade);
        pstmtSync->setString(2, userId);
        pstmtSync->executeUpdate();

        conn->commit(); // 🚀 [Step 2] 트랜잭션 확정
        conn->setAutoCommit(true);

        std::cout << "[MembershipDAO] '" << userId << "' 등급 변경 완료: " << newGrade << std::endl;
        return true;
    }
    catch (sql::SQLException &e)
    {
        conn->rollback();
        conn->setAutoCommit(true);
        std::cerr << "🚨 DB 에러: " << e.what() << std::endl;
        return false;
    }
    catch (const std::exception &e)
    {
        conn->rollback();
        conn->setAutoCommit(true);
        std::cerr << "🚨 일반 에러: " << e.what() << std::endl;
        return false;
    }
}