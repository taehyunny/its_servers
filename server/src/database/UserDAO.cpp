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
            "INSERT INTO USERS (user_id, password_hash, user_name, phone_number, role) VALUES (?, ?, ?, ?, ? )"));
        pstmtUser->setString(1, dto.userId);
        pstmtUser->setString(2, dto.password);
        pstmtUser->setString(3, dto.userName);
        pstmtUser->setString(4, dto.phoneNumber);
        pstmtUser->setInt(5, dto.role);
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
        // 🚀 OWNERS 테이블에 business_number 컬럼이 있다고 가정합니다.
        std::string query = "SELECT COUNT(*) FROM OWNERS WHERE business_number = ?";

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

std::pair<LoginResult, nlohmann::json> UserDAO::checkLogin(const std::string &userId, const std::string &password)
{
    nlohmann::json userJson;
    try
    {
        auto conn = DBManager::getInstance().getConnection();
        std::string query = R"(
            SELECT U.user_id, U.user_name, U.phone_number, U.role, C.address, S.store_name, S.grade
            FROM USERS U
            LEFT JOIN CUSTOMERS C ON U.user_id = C.user_id
            LEFT JOIN STORES S ON U.user_id = S.owner_id
            WHERE U.user_id = ? AND U.password_hash = ?
        )";

        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(query));
        pstmt->setString(1, userId);
        pstmt->setString(2, password);

        std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());

        if (rs->next())
        {
            // 🚀 1. 기본 유저 정보 세팅 (안전한 std::string 캐스팅 적용)
            userJson["user_id"] = std::string(rs->getString("user_id"));
            userJson["user_name"] = std::string(rs->getString("user_name"));
            userJson["phone_number"] = rs->isNull("phone_number") ? "" : std::string(rs->getString("phone_number"));
            userJson["role"] = rs->getInt("role");
            userJson["address"] = rs->isNull("address") ? "" : std::string(rs->getString("address"));
            userJson["store_name"] = rs->isNull("store_name") ? "" : std::string(rs->getString("store_name"));
            userJson["grade"] = rs->isNull("grade") ? 0 : rs->getInt("grade");

            // ---------------------------------------------------------
            // 🚀 로그인 성공 시 메인 화면 데이터 (윗줄/아랫줄/1등매장) 동기화
            // ---------------------------------------------------------

            // 🚀 2. 윗줄: 음식 카테고리 ('편의점', '마트' 제외)
            std::unique_ptr<sql::PreparedStatement> pstmtFood(conn->prepareStatement(
                "SELECT category_id, name FROM CATEGORIES WHERE name NOT IN ('편의점', '마트') ORDER BY category_id ASC"));
            std::unique_ptr<sql::ResultSet> rsFood(pstmtFood->executeQuery());
            
            userJson["foodCategories"] = nlohmann::json::array();
            while (rsFood->next()) {
                userJson["foodCategories"].push_back({
                    {"categoryId", rsFood->getInt("category_id")},
                    {"name", std::string(rsFood->getString("name"))}
                });
            }

            // 🚀 3. 아랫줄: 브랜드 카테고리 (편의점, 마트 통합)
            std::unique_ptr<sql::PreparedStatement> pstmtBrand(conn->prepareStatement(
                "SELECT DISTINCT brand_name FROM STORES WHERE category IN ('편의점', '마트') AND brand_name IS NOT NULL"));
            std::unique_ptr<sql::ResultSet> rsBrand(pstmtBrand->executeQuery());
            
            userJson["brandCategories"] = nlohmann::json::array();
            while (rsBrand->next()) {
                userJson["brandCategories"].push_back({
                    {"brandName", std::string(rsBrand->getString("brand_name"))}
                });
            }

            // 🚀 4. 매출 1등 매장 (새로고침과 동일한 프론트엔드 UI 풀세트!)
            std::unique_ptr<sql::PreparedStatement> pstmtStore(conn->prepareStatement(
                "SELECT store_id, store_name, total_sales, rating, delivery_fee, "
                "min_order_amount, review_count, delivery_time_range, icon_name "
                "FROM STORES ORDER BY total_sales DESC LIMIT 10"));
            std::unique_ptr<sql::ResultSet> rsStore(pstmtStore->executeQuery());

            userJson["topStores"] = nlohmann::json::array();
            while (rsStore->next()) {
                userJson["topStores"].push_back({
                    {"storeId", rsStore->getInt("store_id")},
                    {"storeName", std::string(rsStore->getString("store_name"))},
                    {"totalSales", rsStore->getInt("total_sales")},
                    {"rating", rsStore->getDouble("rating")},
                    {"deliveryFee", rsStore->getInt("delivery_fee")},
                    {"minOrderAmount", rsStore->getInt("min_order_amount")},
                    {"reviewCount", rsStore->getInt("review_count")},
                    {"deliveryTimeRange", std::string(rsStore->getString("delivery_time_range"))},
                    {"iconName", std::string(rsStore->getString("icon_name"))}
                });
            }

            return {LoginResult::SUCCESS, userJson}; // ✅ 성공 코드와 함께 모든 데이터 반환!
        }
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "[UserDAO] 로그인 조회 실패: " << e.what() << std::endl;
    }
    return {LoginResult::ID_PASS_WRONG, nullptr}; // ❌ 실패 시
}

bool UserDAO::executeUpdate(const std::string &query, const std::vector<std::string> &params)
{
    try
    {
        auto conn = DBManager::getInstance().getConnection();
        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(query));

        // 파라미터 개수만큼 순차적으로 바인딩
        for (size_t i = 0; i < params.size(); ++i)
        {
            pstmt->setString(i + 1, params[i]);
        }

        pstmt->executeUpdate();
        return true;
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "[UserDAO] 동적 업데이트 실패: " << e.what() << std::endl;
        return false;
    }
}
std::string UserDAO::getUserGrade(const std::string &userId)
{
    std::string grade = "일반"; // 기본값
    try
    {
        auto conn = DBManager::getInstance().getConnection();
        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
            "SELECT grade FROM USERS WHERE user_id = ?"));
        pstmt->setString(1, userId);
        std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());

        if (rs->next())
        {
            grade = rs->isNull("grade") ? "일반" : rs->getString("grade").c_str();
        }
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "🚨 [UserDAO] 유저 등급 조회 실패: " << e.what() << std::endl;
    }
    return grade;
}