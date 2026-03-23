#include "AuthDAO.h"
#include "DbManager.h"
#include "Global_protocol.h"
#include <iostream>
#include <mariadb/conncpp.hpp>

// AuthDAO는 로그인 검증을 담당하는 DAO입니다.
std::pair<LoginResult, nlohmann::json> AuthDAO::validateLogin(const std::string &inputId, const std::string &inputPw, int role)
{
    std::shared_ptr<sql::Connection> conn;
    nlohmann::json userInfo;

    try
    {
        conn = DBManager::getInstance().getConnection();

        // 🚀 핵심 수정: CUSTOMERS(C) 테이블을 LEFT JOIN에 추가하여 주소(address)를 가져옵니다.
        std::string query = R"(
            SELECT 
                U.password_hash, U.user_name, U.phone_number, U.role,
                C.address,
                O.account_number, O.approval_status, O.business_number,
                S.store_id, S.store_name, S.category, S.store_address,
                S.cook_time, S.min_order_amount, S.open_time, S.close_time
            FROM USERS U
            LEFT JOIN CUSTOMERS C ON U.user_id = C.user_id
            LEFT JOIN OWNERS O ON U.user_id = O.user_id
            LEFT JOIN STORES S ON U.user_id = S.owner_id
            WHERE U.user_id = ? AND U.role = ?
        )";
        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(query));
        pstmt->setString(1, inputId);
        pstmt->setInt(2, role);

        std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());

        // 1. 아이디/역할 체크
        if (!rs->next())
        {
            std::cout << "[AuthDAO] 로그인 실패: 존재하지 않는 아이디 또는 권한 불일치 (" << inputId << ")" << std::endl;
            return {LoginResult::ID_PASS_WRONG, userInfo};
        }

        // 2. 비밀번호 검증
        std::string dbPassword = rs->getString("password_hash").c_str();
        if (dbPassword != inputPw)
        {
            std::cout << "[AuthDAO] 로그인 실패: 비밀번호 불일치 (" << inputId << ")" << std::endl;
            return {LoginResult::ID_PASS_WRONG, userInfo};
        }

        // 🚀 3. 공통 데이터 세팅 (모든 역할 공통)
        userInfo["userId"] = inputId;
        userInfo["userName"] = rs->getString("user_name").c_str();
        userInfo["phoneNumber"] = rs->isNull("phone_number") ? "" : rs->getString("phone_number").c_str();
        userInfo["role"] = std::to_string(rs->getInt("role"));
        userInfo["status"] = 200;
        userInfo["message"] = "로그인 성공";

        // 🚀 4. 일반 고객(role == 0) 데이터 세팅
        if (rs->getInt("role") == 0)
        {
            // CUSTOMERS 테이블에서 가져온 주소 매핑
            userInfo["address"] = rs->isNull("address") ? "" : rs->getString("address").c_str();
        }

        // 🚀 5. 사장님(role == 1) 데이터 세팅
        if (rs->getInt("role") == 1)
        {
            userInfo["storeId"] = rs->isNull("store_id") ? 0 : rs->getInt("store_id");
            userInfo["storeName"] = rs->isNull("store_name") ? "" : rs->getString("store_name").c_str();
            userInfo["category"] = rs->isNull("category") ? "" : rs->getString("category").c_str();
            userInfo["storeAddress"] = rs->isNull("store_address") ? "" : rs->getString("store_address").c_str();

            // INT -> String 변환 (DTO 규격 준수)
            userInfo["cookTime"] = rs->isNull("cook_time") ? "0" : std::to_string(rs->getInt("cook_time"));
            userInfo["minOrderAmount"] = rs->isNull("min_order_amount") ? "0" : std::to_string(rs->getInt("min_order_amount"));

            userInfo["openTime"] = rs->isNull("open_time") ? "" : rs->getString("open_time").c_str();
            userInfo["closeTime"] = rs->isNull("close_time") ? "" : rs->getString("close_time").c_str();

            userInfo["accountNumber"] = rs->isNull("account_number") ? "" : rs->getString("account_number").c_str();
            userInfo["businessNumber"] = rs->isNull("business_number") ? "" : rs->getString("business_number").c_str();
            userInfo["approvalStatus"] = rs->isNull("approval_status") ? 0 : rs->getInt("approval_status");
        }

        std::cout << "[AuthDAO] 로그인 성공 (" << inputId << "), Role: " << rs->getInt("role") << std::endl;
        return {LoginResult::SUCCESS, userInfo};
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "[AuthDAO] DB 오류: " << e.what() << std::endl;
        return {LoginResult::SERVER_ERROR, userInfo};
    }
}