#include "UserDAO.h"
#include "DbManager.h"
#include "Global_protocol.h"
#include "AllDTOs.h"
#include <iostream>

#include <mariadb/conncpp.hpp>

bool UserDAO::existsById(const std::string &userId)
{
    try
    {
        auto conn = DBManager::getInstance().getConnection();
        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
            "SELECT COUNT(*) FROM USERS WHERE user_id = ?"));
        pstmt->setString(1, userId);
        std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());

        if (rs->next())
            return rs->getInt(1) > 0;
    }
    catch (...)
    {
    }
    return false;
}

// 🚀 주소(Address)는 보통 유일할 필요가 없지만, 팀원들이 원한다면 똑같이 만듭니다.
bool UserDAO::existsByPhoneAndRole(const std::string &phoneNumber, int role)
{
    try
    {
        auto conn = DBManager::getInstance().getConnection();
        // 🚀 폰번호와 역할이 둘 다 똑같은 계정이 있는지 카운트!
        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
            "SELECT COUNT(*) FROM USERS WHERE phone_number = ? AND role = ?"));
        pstmt->setString(1, phoneNumber);
        pstmt->setInt(2, role);
        std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());

        if (rs->next())
            return rs->getInt(1) > 0;
    }
    catch (...)
    {
    }
    return false;
}

SignupResult UserDAO::insertUser(const SignupReqDTO &dto)
{
    std::shared_ptr<sql::Connection> conn;

    try
    {
        conn = DBManager::getInstance().getConnection();

        // ---------------------------------------------------------
        // [0단계] 사전 검문 (아이디 & 전화번호 중복 체크)
        // ---------------------------------------------------------
        std::unique_ptr<sql::PreparedStatement> pstmtCheckId(conn->prepareStatement(
            "SELECT COUNT(*) FROM USERS WHERE user_id = ?"));
        pstmtCheckId->setString(1, dto.userId);
        std::unique_ptr<sql::ResultSet> rsId(pstmtCheckId->executeQuery());
        if (rsId->next() && rsId->getInt(1) > 0)
            return SignupResult::DUPLICATE_ID;

        std::unique_ptr<sql::PreparedStatement> pstmtCheckPhone(conn->prepareStatement(
            "SELECT COUNT(*) FROM USERS WHERE phone_number = ? AND role = ?"));
        pstmtCheckPhone->setString(1, dto.phoneNumber);
        pstmtCheckPhone->setInt(2, dto.role);
        std::unique_ptr<sql::ResultSet> rsPhone(pstmtCheckPhone->executeQuery());
        if (rsPhone->next() && rsPhone->getInt(1) > 0)
            return SignupResult::DUPLICATE_PHONE;

        // ---------------------------------------------------------
        // [1단계] 트랜잭션 시작 및 USERS Insert
        // ---------------------------------------------------------
        conn->setAutoCommit(false);

        std::unique_ptr<sql::PreparedStatement> pstmtUser(conn->prepareStatement(
            "INSERT INTO USERS (user_id, password_hash, user_name, phone_number, role, business_number) VALUES (?, ?, ?, ?, ?, ?, ?)"));
        pstmtUser->setString(1, dto.userId);
        pstmtUser->setString(2, dto.password);
        pstmtUser->setString(3, dto.userName);
        pstmtUser->setString(4, dto.phoneNumber);
        pstmtUser->setInt(5, dto.role);
        pstmtUser->setString(6, dto.businessNumber);
        pstmtUser->executeUpdate();

        // ---------------------------------------------------------
        // [2단계] 역할별 테이블 Insert
        // ---------------------------------------------------------
        if (dto.role == 0)
        {
            std::unique_ptr<sql::PreparedStatement> pstmtCustomer(conn->prepareStatement(
                "INSERT INTO CUSTOMERS (user_id, address) VALUES (?, ?)"));
            pstmtCustomer->setString(1, dto.userId);
            pstmtCustomer->setString(2, dto.address.empty() ? "주소 미상" : dto.address);
            pstmtCustomer->executeUpdate();
        }
        else if (dto.role == 1)
        {
            std::unique_ptr<sql::PreparedStatement> pstmtOwner(conn->prepareStatement(
                "INSERT INTO OWNERS (user_id, business_number, account_number) VALUES (?, ?, ?)"));
            pstmtOwner->setString(1, dto.userId);
            pstmtOwner->setString(2, dto.businessNumber.empty() ? "미등록" : dto.businessNumber);
            pstmtOwner->setString(3, dto.accountNumber.empty() ? "미등록" : dto.accountNumber);
            pstmtOwner->executeUpdate();

            std::unique_ptr<sql::PreparedStatement> pstmtStore(conn->prepareStatement(
                "INSERT INTO STORES (owner_id, store_name, category, store_address) VALUES (?, ?, ?, ?)"));
            pstmtStore->setString(1, dto.userId);
            pstmtStore->setString(2, dto.storeName.empty() ? "가게명 미상" : dto.storeName);
            pstmtStore->setString(3, dto.category.empty() ? "기타" : dto.category);
            pstmtStore->setString(4, dto.storeAddress.empty() ? "주소 미상" : dto.storeAddress);
            pstmtStore->executeUpdate();
        }

        // ---------------------------------------------------------
        // [3단계] Commit
        // ---------------------------------------------------------
        conn->commit();
        conn->setAutoCommit(true);

        std::cout << "[UserDAO] 회원가입 성공 (ID: " << dto.userId << ")" << std::endl;
        return SignupResult::SUCCESS;
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "[UserDAO] 회원가입 실패 (Rollback): " << e.what() << std::endl;
        if (conn)
        {
            try
            {
                conn->rollback();
                conn->setAutoCommit(true);
            }
            catch (sql::SQLException &rollbackEx)
            {
                std::cerr << "[FATAL] 롤백 실패: " << rollbackEx.what() << std::endl;
            }
        }
        return SignupResult::SERVER_ERROR;
    }
}

bool UserDAO::existsByBizNum(const std::string &businessNum)
{
    try
    {
        auto conn = DBManager::getInstance().getConnection();
        // 🚀 USERS 테이블에 business_num 컬럼이 있다고 가정합니다.
        std::string query = "SELECT COUNT(*) FROM USERS WHERE business_num = ?";

        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(query));
        pstmt->setString(1, businessNum);

        std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());

        if (rs->next())
        {
            return rs->getInt(1) > 0; // 0보다 크면 이미 존재하는 것!
        }
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "[UserDAO] 사업자 번호 중복 체크 실패: " << e.what() << std::endl;
    }
    return false;
}