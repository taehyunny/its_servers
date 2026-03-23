#include "AddressDAO.h"
#include <iostream>

// ── 1. 주소 저장 ──────────────────────────────────────
int AddressDAO::saveAddress(const ReqAddressSaveDTO& req) {
    try {
        auto conn = DBManager::getInstance().getConnection();
        
        // 🚀 비즈니스 로직: 무조건 새로 추가하는 주소를 기본 주소(is_default=1)로 만듭니다.
        // 먼저 이 유저의 기존 주소들을 전부 0으로 강등시킵니다.
        std::unique_ptr<sql::PreparedStatement> pstmtUpdate(
            conn->prepareStatement("UPDATE USER_ADDRESSES SET is_default = 0 WHERE user_id = ?")
        );
        pstmtUpdate->setString(1, req.userId);
        pstmtUpdate->executeUpdate();

        // 그 다음 새 주소를 1(기본)로 INSERT 합니다.
        std::unique_ptr<sql::PreparedStatement> pstmtInsert(
            conn->prepareStatement(
                "INSERT INTO USER_ADDRESSES (user_id, address, detail, guide, label, is_default) "
                "VALUES (?, ?, ?, ?, ?, 1)"
            )
        );
        pstmtInsert->setString(1, req.userId);
        pstmtInsert->setString(2, req.address);
        pstmtInsert->setString(3, req.detail);
        pstmtInsert->setString(4, req.guide);
        pstmtInsert->setString(5, req.label);
        pstmtInsert->executeUpdate();

        // 방금 넣은 주소의 ID(PK)를 가져옵니다.
        std::unique_ptr<sql::Statement> stmt(conn->createStatement());
        std::unique_ptr<sql::ResultSet> rs(stmt->executeQuery("SELECT LAST_INSERT_ID()"));
        if (rs->next()) {
            return rs->getInt(1);
        }
    } catch (sql::SQLException &e) {
        std::cerr << "[AddressDAO] 주소 저장 실패: " << e.what() << std::endl;
    }
    return 0;
}

// ── 2. 주소 목록 조회 ──────────────────────────────────
std::vector<AddressItem> AddressDAO::getAddressList(const std::string& userId) {
    std::vector<AddressItem> list;
    try {
        auto conn = DBManager::getInstance().getConnection();
        std::unique_ptr<sql::PreparedStatement> pstmt(
            conn->prepareStatement(
                "SELECT address_id, address, detail, guide, label, is_default "
                "FROM USER_ADDRESSES WHERE user_id = ? ORDER BY is_default DESC, address_id DESC"
            ) // 기본 주소가 맨 위로, 그 다음은 최신순으로 정렬
        );
        pstmt->setString(1, userId);
        std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());

        while (rs->next()) {
            AddressItem item;
            item.addressId = rs->getInt("address_id");
            item.address   = rs->getString("address").c_str();
            item.detail    = rs->getString("detail").c_str();
            item.guide     = rs->getString("guide").c_str();
            item.label     = rs->getString("label").c_str();
            item.isDefault = (rs->getInt("is_default") == 1);
            list.push_back(item);
        }
    } catch (sql::SQLException &e) {
        std::cerr << "[AddressDAO] 주소 목록 조회 실패: " << e.what() << std::endl;
    }
    return list;
}