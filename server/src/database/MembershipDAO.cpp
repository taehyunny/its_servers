#include "MembershipDAO.h"
#include "DbManager.h"
#include "AllDTOs.h" // DTO 정의가 포함된 헤더
#include <iostream>
#include <mariadb/conncpp.hpp> // 🚀 모든 상세 명세가 들어있는 헤더

bool MembershipDAO::updateMembershipGrade(const std::string &userId, const std::string &targetGrade)
{
    auto conn = DBManager::getInstance().getConnection();
    try
    {
        // 🚀 1. 트랜잭션 시작 (둘 다 성공해야만 인정!)
        conn->setAutoCommit(false);

        // 🚀 2. CUSTOMERS 테이블 업데이트 (현재 등급 즉시 변경)
        std::unique_ptr<sql::PreparedStatement> pstmt1(conn->prepareStatement(
            "UPDATE CUSTOMERS SET customer_grade = ? WHERE user_id = ?"));
        pstmt1->setString(1, targetGrade);
        pstmt1->setString(2, userId);
        pstmt1->executeUpdate();

        // 🚀 3. USER_MEMBERSHIPS 테이블에 결제 이력 INSERT
        // (MariaDB의 DATE_ADD 함수로 정확히 한 달 뒤를 계산합니다)
        std::unique_ptr<sql::PreparedStatement> pstmt2(conn->prepareStatement(
            "INSERT INTO USER_MEMBERSHIPS (user_id, grade_name, start_date, end_date, is_active) "
            "VALUES (?, ?, NOW(), DATE_ADD(NOW(), INTERVAL 1 MONTH), 1)")); // 👈 여기서 한 번만 닫아야 합니다!

        pstmt2->setString(1, userId);
        pstmt2->setString(2, targetGrade);
        pstmt2->executeUpdate();

        // 🚀 4. 모든 것이 완벽하면 영구 반영!
        conn->commit();
        conn->setAutoCommit(true);
        return true;
    }
    catch (const sql::SQLException &e)
    {
        std::cerr << "🚨 [MembershipDAO] 등급 변경 트랜잭션 실패: " << e.what() << std::endl;
        // 에러 나면 싹 다 없던 일로!
        if (conn)
        {
            conn->rollback();
            conn->setAutoCommit(true);
        }
        return false;
    }
}