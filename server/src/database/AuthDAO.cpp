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

        // 🚀 핵심 수정: 낡은 CUSTOMERS 테이블 대신 USER_ADDRESSES(UA)와 JOIN 합니다.
        // UA.is_default = 1 조건을 추가하여 현재 선택된 기본 주소만 타겟팅합니다.
        std::string query = R"(
            SELECT 
                U.password_hash, U.user_name, U.phone_number, U.role, U.grade,
                UA.address, UA.detail,
                O.account_number, O.approval_status, O.business_number,
                S.store_id, S.store_name, S.category, S.store_address,
                S.cook_time, S.min_order_amount, S.open_time, S.close_time
            FROM USERS U
            LEFT JOIN USER_ADDRESSES UA ON U.user_id = UA.user_id AND UA.is_default = 1
            LEFT JOIN OWNERS O ON U.user_id = O.user_id
            LEFT JOIN STORES S ON U.user_id = S.owner_id
            WHERE U.user_id = ? AND U.role = ?
            LIMIT 1
        )";

        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(query));
        pstmt->setString(1, inputId);
        pstmt->setInt(2, role);

        std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());

        // 1. 아이디/역할 체크
        if (!rs->next())
        {
            std::cout << "[AuthDAO] 로그인 실패: 아이디 불일치 또는 권한 미흡 (" << inputId << ")" << std::endl;
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

        // 🚀 DB에서 꺼낸 grade를 JSON에 예쁘게 담아줍니다!
        userInfo["grade"] = rs->isNull("grade") ? "일반" : rs->getString("grade").c_str();

        userInfo["status"] = 200;
        userInfo["message"] = "로그인 성공";

        userInfo["address"] = ""; // 기본값 세팅 (주소는 role에 따라 다르게 채워질 예정)
        // 🚀 4. 일반 고객(role == 0) 데이터 세팅
        if (rs->getInt("role") == 0)
        {
            // UA 테이블에서 가져온 기본 주소와 상세 주소를 결합
            std::string mainAddr = rs->isNull("address") ? "" : rs->getString("address").c_str();
            std::string detailAddr = rs->isNull("detail") ? "" : rs->getString("detail").c_str();

            if (!mainAddr.empty() && !detailAddr.empty())
            {
                userInfo["address"] = mainAddr + " " + detailAddr; // "에버랜드로 143 + 1층"
            }
            else
            {
                userInfo["address"] = mainAddr;
            }
        }

        // 🚀 5. 사장님(role == 1) 데이터 세팅
        if (rs->getInt("role") == 1)
        {
            userInfo["storeId"] = rs->isNull("store_id") ? 0 : rs->getInt("store_id");
            userInfo["storeName"] = rs->isNull("store_name") ? "" : rs->getString("store_name").c_str();
            userInfo["category"] = rs->isNull("category") ? "" : rs->getString("category").c_str();
            userInfo["storeAddress"] = rs->isNull("store_address") ? "" : rs->getString("store_address").c_str();

            userInfo["cookTime"] = rs->isNull("cook_time") ? "0" : std::to_string(rs->getInt("cook_time"));
            userInfo["minOrderAmount"] = rs->isNull("min_order_amount") ? "0" : std::to_string(rs->getInt("min_order_amount"));

            userInfo["openTime"] = rs->isNull("open_time") ? "" : rs->getString("open_time").c_str();
            userInfo["closeTime"] = rs->isNull("close_time") ? "" : rs->getString("close_time").c_str();

            userInfo["accountNumber"] = rs->isNull("account_number") ? "" : rs->getString("account_number").c_str();
            userInfo["businessNumber"] = rs->isNull("business_number") ? "" : rs->getString("business_number").c_str();
            userInfo["approvalStatus"] = rs->isNull("approval_status") ? 0 : rs->getInt("approval_status");
        }

        std::cout << "[AuthDAO] 로그인 성공 (" << inputId << "), Address: " << userInfo["address"] << std::endl;
        return {LoginResult::SUCCESS, userInfo};
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "[AuthDAO] DB 오류: " << e.what() << std::endl;
        return {LoginResult::SERVER_ERROR, userInfo};
    }
}