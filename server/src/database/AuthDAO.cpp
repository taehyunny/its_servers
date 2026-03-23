#include "AuthDAO.h"
#include "DbManager.h"
#include "Global_protocol.h"
#include <iostream>
#include <mariadb/conncpp.hpp>

// AuthDAO는 로그인 검증을 담당하는 DAO입니다.
std::pair<LoginResult, nlohmann::json> validateLogin(const std::string &inputId, const std::string &inputPw, int role) // 🚀 role 추가
{
    std::shared_ptr<sql::Connection> conn;
    nlohmann::json userInfo;

    try
    {
        conn = DBManager::getInstance().getConnection();

        // 🚀 1. USERS, OWNERS, STORES 3단 LEFT JOIN 쿼리!
        // 사장님이 아니면 OWNERS와 STORES 데이터는 알아서 NULL이 됩니다.
        // 🚀 1. SELECT 항목에 O.business_number 추가!
        std::string query = R"(
        SELECT 
            U.password_hash, U.user_name, U.phone_number, U.role,
            O.account_number, O.approval_status, O.business_number,
            S.store_id, S.store_name, S.category, S.store_address,
            S.cook_time, S.min_order_amount, S.open_time, S.close_time
        FROM USERS U
        LEFT JOIN OWNERS O ON U.user_id = O.user_id
        LEFT JOIN STORES S ON U.user_id = S.owner_id
        WHERE U.user_id = ? AND U.role = ?   -- 🚀 핵심: 역할(role)까지 완벽하게 일치해야 함!
    )";

        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(query));
        pstmt->setString(1, inputId);
        pstmt->setInt(2, role);

        std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());

        // 결과가 없다면 아이디가 틀린(없는) 것
        if (!rs->next())
        {
            std::cout << "[AuthDAO] 로그인 실패: 존재하지 않는 아이디 (" << inputId << ")" << std::endl;
            return {LoginResult::ID_PASS_WRONG, userInfo};
        }

        // 2. 비밀번호 검증
        std::string dbPassword = rs->getString("password_hash").c_str();
        if (dbPassword != inputPw)
        {
            std::cout << "[AuthDAO] 로그인 실패: 비밀번호 불일치 (" << inputId << ")" << std::endl;
            return {LoginResult::ID_PASS_WRONG, userInfo};
        }

        // 🚀 3. 인증 성공 시 데이터 세팅 (AuthResDTO 변수명과 일치시키기)
        userInfo["userName"] = rs->getString("user_name").c_str();
        userInfo["phoneNumber"] = rs->isNull("phone_number") ? "" : rs->getString("phone_number").c_str();
        // DTO에서 role이 std::string이므로 문자열로 변환해서 넣어줍니다.
        userInfo["role"] = std::to_string(rs->getInt("role"));
        userInfo["status"] = 200; // 성공 상태 코드
        userInfo["message"] = "로그인 성공";

        // 🚀 4. 사장님(role == 1)인 경우에만 추가 정보 세팅!
        if (rs->getInt("role") == 1)
        {
            userInfo["storeId"] = rs->isNull("store_id") ? 0 : rs->getInt("store_id");
            userInfo["storeName"] = rs->isNull("store_name") ? "" : rs->getString("store_name").c_str();
            userInfo["category"] = rs->isNull("category") ? "" : rs->getString("category").c_str();
            userInfo["storeAddress"] = rs->isNull("store_address") ? "" : rs->getString("store_address").c_str();

            // 🚨 핵심 수정: DB(INT) -> C++(String) 변환
            userInfo["cookTime"] = rs->isNull("cook_time") ? "0" : std::to_string(rs->getInt("cook_time"));
            userInfo["minOrderAmount"] = rs->isNull("min_order_amount") ? "0" : std::to_string(rs->getInt("min_order_amount"));

            userInfo["openTime"] = rs->isNull("open_time") ? "" : rs->getString("open_time").c_str();
            userInfo["closeTime"] = rs->isNull("close_time") ? "" : rs->getString("close_time").c_str();

            // (OWNERS 테이블 데이터)
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