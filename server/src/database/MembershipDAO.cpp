#include "MembershipDAO.h"
#include "DbManager.h"
#include <iostream>
#include <mariadb/conncpp.hpp>

// 🚀 파라미터에 int action 추가 (1: 구독, 2: 해지)
bool MembershipDAO::updateMembershipGrade(const std::string &userId, const std::string &targetGrade, int action)
{
    auto conn = DBManager::getInstance().getConnection();
    try
    {
        // 1. 트랜잭션 시작 (3개 테이블 모두 성공해야만 인정!)
        conn->setAutoCommit(false);

        if (action == 1)
        {
            // ==========================================
            // [신규 구독 로직]
            // ==========================================
            // 🚀 1-A. custumers 테이블 업데이트 (오타 주의!)
            std::unique_ptr<sql::PreparedStatement> pstmt1(conn->prepareStatement(
                "UPDATE custumers SET customer_grade = ? WHERE user_id = ?"));
            pstmt1->setString(1, targetGrade); // "와우"
            pstmt1->setString(2, userId);
            pstmt1->executeUpdate();

            // 🚀 1-B. USERS 테이블 업데이트 (새로 추가된 동기화 로직!)
            std::unique_ptr<sql::PreparedStatement> pstmtUsers(conn->prepareStatement(
                "UPDATE USERS SET grade = ? WHERE user_id = ?"));
            pstmtUsers->setString(1, targetGrade); // "와우"
            pstmtUsers->setString(2, userId);
            pstmtUsers->executeUpdate();

            // 🚀 1-C. USER_MEMBERSHIPS 테이블 이력 추가
            std::unique_ptr<sql::PreparedStatement> pstmt2(conn->prepareStatement(
                "INSERT INTO USER_MEMBERSHIPS (user_id, grade_name, start_date, is_active) "
                "VALUES (?, ?, NOW(), 1)"));
            pstmt2->setString(1, userId);
            pstmt2->setString(2, targetGrade);
            pstmt2->executeUpdate();

            std::cout << "[MembershipDAO] 💎 " << userId << " 님의 " << targetGrade << " 멤버십 구독 완료! (USERS 테이블 동기화 완료)" << std::endl;
        }
        else if (action == 2)
        {
            // ==========================================
            // [구독 해지 로직]
            // ==========================================
            // 🚀 2-A. custumers 테이블 롤백
            std::unique_ptr<sql::PreparedStatement> pstmt1(conn->prepareStatement(
                "UPDATE custumers SET customer_grade = '일반' WHERE user_id = ?"));
            pstmt1->setString(1, userId);
            pstmt1->executeUpdate();

            // 🚀 2-B. USERS 테이블 롤백 (새로 추가된 동기화 로직!)
            std::unique_ptr<sql::PreparedStatement> pstmtUsers(conn->prepareStatement(
                "UPDATE USERS SET grade = '일반' WHERE user_id = ?"));
            pstmtUsers->setString(1, userId);
            pstmtUsers->executeUpdate();

            // 🚀 2-C. USER_MEMBERSHIPS 비활성화
            std::unique_ptr<sql::PreparedStatement> pstmt2(conn->prepareStatement(
                "UPDATE USER_MEMBERSHIPS SET is_active = 0 "
                "WHERE user_id = ? AND is_active = 1"));
            pstmt2->setString(1, userId);
            pstmt2->executeUpdate();

            std::cout << "[MembershipDAO] 💔 " << userId << " 님의 멤버십 해지 완료. (USERS 테이블 동기화 완료)" << std::endl;
        }
        else
        {
            throw std::runtime_error("알 수 없는 Action 코드입니다.");
        }

        // 모든 것이 완벽하면 영구 반영!
        conn->commit();
        conn->setAutoCommit(true);
        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [MembershipDAO] 등급 변경 트랜잭션 실패: " << e.what() << std::endl;
        if (conn)
        {
            conn->rollback();
            conn->setAutoCommit(true);
        }
        return false;
    }
}