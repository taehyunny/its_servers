#include "AddressDAO.h"
#include <iostream>
#include <mariadb/conncpp.hpp> // MariaDB 사용 시 필수

// ── 1. 주소 저장 ──────────────────────────────────────
int AddressDAO::saveAddress(const ReqAddressSaveDTO &dto)
{
    auto conn = DBManager::getInstance().getConnection();
    try
    {
        conn->setAutoCommit(false); // 트랜잭션 시작

        // 🚀 [추가] 0. 기존에 설정된 기본 주소들을 모두 해제 (0으로 초기화)
        // 한 명의 유저당 '선택됨(1)'은 하나만 존재해야 하니까요!
        std::unique_ptr<sql::PreparedStatement> pstmtReset(conn->prepareStatement(
            "UPDATE USER_ADDRESSES SET is_default = 0 WHERE user_id = ?"));
        pstmtReset->setString(1, dto.userId);
        pstmtReset->executeUpdate();

        // 🚀 1. 새 주소를 '기본 주소(is_default = 1)'로 저장
        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
            "INSERT INTO USER_ADDRESSES (user_id, address, detail, guide, label, is_default) "
            "VALUES (?, ?, ?, ?, ?, ?)"));

        pstmt->setString(1, dto.userId);
        pstmt->setString(2, dto.address);
        pstmt->setString(3, dto.detail);
        pstmt->setString(4, dto.guide);
        pstmt->setString(5, dto.label);
        pstmt->setInt(6, 1); // 🚀 0에서 1로 변경! (이제 '선택됨'이 뜹니다)

        pstmt->executeUpdate();

        // (이하 ID 가져오기 및 CUSTOMERS 테이블 동기화 로직은 동일...)
        int newId = 0;
        std::unique_ptr<sql::Statement> stmt(conn->createStatement());
        std::unique_ptr<sql::ResultSet> rs(stmt->executeQuery("SELECT LAST_INSERT_ID()"));
        if (rs->next())
            newId = rs->getInt(1);

        std::string fullAddr = dto.address + " " + dto.detail;
        std::unique_ptr<sql::PreparedStatement> pstmtUser(conn->prepareStatement(
            "UPDATE CUSTOMERS SET address = ? WHERE user_id = ?"));
        pstmtUser->setString(1, fullAddr);
        pstmtUser->setString(2, dto.userId);
        pstmtUser->executeUpdate();

        conn->commit();
        conn->setAutoCommit(true);
        return newId;
    }
    catch (const std::exception &e)
    {
        conn->rollback();
        conn->setAutoCommit(true);
        std::cerr << "🚨 [AddressDAO] saveAddress 에러: " << e.what() << std::endl;
        return -1;
    }
}

// ── 2. 주소 목록 조회 ──────────────────────────────────
std::vector<AddressItemDTO> AddressDAO::getAddressList(const std::string &userId)
{
    std::vector<AddressItemDTO> list;
    try
    {
        auto conn = DBManager::getInstance().getConnection();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            conn->prepareStatement(
                "SELECT address_id, address, detail, guide, label, is_default "
                "FROM USER_ADDRESSES WHERE user_id = ? ORDER BY is_default DESC, address_id DESC") // 기본 주소가 맨 위로, 그 다음은 최신순으로 정렬
        );
        pstmt->setString(1, userId);
        std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());

        while (rs->next())
        {
            AddressItemDTO item;
            item.addressId = rs->getInt("address_id");
            item.address = rs->getString("address").c_str();
            item.detail = rs->getString("detail").c_str();
            item.guide = rs->getString("guide").c_str();
            item.label = rs->getString("label").c_str();
            item.isDefault = (rs->getInt("is_default") == 1);
            list.push_back(item);
        }
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "[AddressDAO] 주소 목록 조회 실패: " << e.what() << std::endl;
    }
    return list;
}

// ── 3. 주소 삭제 (기본 주소는 삭제 불가 로직 포함) ────────────────
bool AddressDAO::deleteAddress(const std::string &userId, int addressId)
{
    try
    {
        auto conn = DBManager::getInstance().getConnection();

        // 1. 먼저 이 주소가 기본 주소(is_default=1)인지 확인
        std::unique_ptr<sql::PreparedStatement> pstmtCheck(
            conn->prepareStatement("SELECT is_default FROM USER_ADDRESSES WHERE user_id = ? AND address_id = ?"));
        pstmtCheck->setString(1, userId);
        pstmtCheck->setInt(2, addressId);
        std::unique_ptr<sql::ResultSet> rs(pstmtCheck->executeQuery());

        if (rs->next())
        {
            if (rs->getInt("is_default") == 1)
            {
                std::cout << "[AddressDAO] 기본 주소는 삭제할 수 없습니다." << std::endl;
                return false; // 프론트의 요청대로 삭제 불가 처리!
            }
        }
        else
        {
            return false; // 없는 주소
        }

        // 2. 기본 주소가 아니라면 삭제 진행
        std::unique_ptr<sql::PreparedStatement> pstmtDelete(
            conn->prepareStatement("DELETE FROM USER_ADDRESSES WHERE user_id = ? AND address_id = ?"));
        pstmtDelete->setString(1, userId);
        pstmtDelete->setInt(2, addressId);
        int rows = pstmtDelete->executeUpdate();

        return rows > 0;
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "[AddressDAO] 삭제 에러: " << e.what() << std::endl;
        return false;
    }
}

// ── 4. 기본 주소 변경 ──────────────────────────────────
bool AddressDAO::setDefaultAddress(const std::string &userId, int addressId)
{
    try
    {
        // 🚀 0. DB 연결 가져오기 (이게 빠져서 에러가 났던 겁니다!)
        auto conn = DBManager::getInstance().getConnection();

        // ─── [원래 있던 핵심 로직] ──────────────────────────
        // 1. 기존에 1이었던 애들을 전부 0으로 강등
        std::unique_ptr<sql::PreparedStatement> pstmtReset(
            conn->prepareStatement("UPDATE USER_ADDRESSES SET is_default = 0 WHERE user_id = ?"));
        pstmtReset->setString(1, userId);
        pstmtReset->executeUpdate();

        // 2. 요청받은 주소만 1로 세팅
        std::unique_ptr<sql::PreparedStatement> pstmtSet(
            conn->prepareStatement("UPDATE USER_ADDRESSES SET is_default = 1 WHERE user_id = ? AND address_id = ?"));
        pstmtSet->setString(1, userId);
        pstmtSet->setInt(2, addressId);
        int rows = pstmtSet->executeUpdate();
        // ────────────────────────────────────────────────────

        // ─── [새로 추가된 CUSTOMERS 동기화 로직] ────────────
        if (rows > 0)
        {
            // 3. 방금 is_default = 1로 바꾼 주소의 실제 문자열(address)을 알아냅니다.
            std::unique_ptr<sql::PreparedStatement> pstmtGetAddr(
                conn->prepareStatement("SELECT address FROM USER_ADDRESSES WHERE address_id = ?"));
            pstmtGetAddr->setInt(1, addressId);
            std::unique_ptr<sql::ResultSet> rs(pstmtGetAddr->executeQuery());

            std::string newDefaultAddress = "";
            if (rs->next())
            {
                newDefaultAddress = rs->getString("address").c_str();
            }

            // 4. 알아낸 주소로 CUSTOMERS 테이블을 업데이트 (동기화) 합니다.
            std::unique_ptr<sql::PreparedStatement> pstmtSync(
                conn->prepareStatement("UPDATE CUSTOMERS SET address = ? WHERE user_id = ?"));
            pstmtSync->setString(1, newDefaultAddress);
            pstmtSync->setString(2, userId);
            pstmtSync->executeUpdate();
        }
        // ────────────────────────────────────────────────────

        return rows > 0;
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "[AddressDAO] 기본 주소 변경 에러: " << e.what() << std::endl;
        return false;
    }
}

// ── 5. 주소 수정 ──────────────────────────────────────
bool AddressDAO::updateAddress(const ReqAddressUpdateDTO &req)
{
    try
    {
        auto conn = DBManager::getInstance().getConnection();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            conn->prepareStatement("UPDATE USER_ADDRESSES SET detail = ?, guide = ?, label = ? WHERE user_id = ? AND address_id = ?"));
        pstmt->setString(1, req.detail);
        pstmt->setString(2, req.guide);
        pstmt->setString(3, req.label);
        pstmt->setString(4, req.userId);
        pstmt->setInt(5, req.addressId);

        int rows = pstmt->executeUpdate();
        return rows > 0;
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "[AddressDAO] 주소 수정 에러: " << e.what() << std::endl;
        return false;
    }
}