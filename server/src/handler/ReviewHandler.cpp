#include "ReviewHandler.h"
#include "ClientSession.h"
#include "AllDTOs.h"
#include "DbManager.h" // 🚀 핵심 수정: DbManager -> DBManager 대소문자 수정!
#include "Global_protocol.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <mariadb/conncpp.hpp>

using nlohmann::json;

// =========================================================
// ① 리뷰 목록 조회 (REQ_REVIEW_LIST = 2014)
// =========================================================
// =========================================================
// ① 가게 전체 리뷰 목록 조회 (REQ_REVIEW_LIST = 2014)
// =========================================================
void ReviewHandler::handleReviewList(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    json req = json::parse(jsonBody);
    json res;
    try
    {
        int storeId = req.value("storeId", 0);
        int menuId = req.value("menuId", 0);

        // 둘 다 없으면 에러 처리
        if (storeId == 0 && menuId == 0)
        {
            res["status"] = 400;
            res["message"] = "storeId 또는 menuId가 필요합니다.";
            session->sendPacket(static_cast<uint16_t>(CmdID::RES_REVIEW_LIST), res);
            return;
        }

        auto conn = DBManager::getInstance().getConnection();

        // 🚀 1. 마법의 동적 JOIN 쿼리 (가게 전체 리뷰 vs 특정 메뉴 리뷰 완벽 대응)
        std::string query = "SELECT R.review_id, R.user_id, R.rating, R.content, R.owner_reply, R.created_at "
                            "FROM REVIEWS R ";

        if (menuId > 0)
        {
            query += "JOIN ORDER_ITEMS OI ON R.order_id = OI.order_id ";
        }
        query += "WHERE 1=1 ";

        if (storeId > 0 && menuId == 0)
        {
            query += "AND R.store_id = ? ";
        }
        if (menuId > 0)
        {
            query += "AND OI.menu_id = ? ";
        }
        query += "ORDER BY R.created_at DESC";

        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(query));

        // 🚀 2. 동적 파라미터 바인딩
        int paramIndex = 1;
        if (storeId > 0 && menuId == 0)
        {
            pstmt->setInt(paramIndex++, storeId);
        }
        if (menuId > 0)
        {
            pstmt->setInt(paramIndex++, menuId);
        }

        std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());

        json reviews = json::array();
        while (rs->next())
        {
            json rv;
            rv["reviewId"] = rs->getInt("review_id");
            rv["userId"] = rs->getString("user_id").c_str();
            rv["rating"] = rs->getInt("rating");
            rv["content"] = rs->getString("content").c_str();
            rv["ownerReply"] = rs->isNull("owner_reply") ? "" : rs->getString("owner_reply").c_str();
            rv["createdAt"] = rs->getString("created_at").c_str();
            reviews.push_back(rv);
        }

        res["status"] = 200;
        res["message"] = "조회 성공";

        // 🚀 3. 클라이언트가 데이터를 무시하지 않도록 메아리(Echo) 쳐주기!
        res["storeId"] = storeId;
        res["menuId"] = menuId; // 👈 이 한 줄이 쟁반짜장 리뷰를 살려냅니다!
        res["reviews"] = reviews;

        session->sendPacket(static_cast<uint16_t>(CmdID::RES_REVIEW_LIST), res);
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [ReviewHandler] handleReviewList 오류: " << e.what() << std::endl;
        res["status"] = 500;
        res["message"] = "서버 내부 오류";
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_REVIEW_LIST), res);
    }
}
// =========================================================
// ② 답글 등록 (REQ_REVIEW_REPLY = 3070)
// =========================================================
void ReviewHandler::handleReviewReply(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    json req = json::parse(jsonBody);
    json res;
    try
    {
        int reviewId = req.value("reviewId", 0);
        int storeId = req.value("storeId", 0);
        std::string ownerReply = req.value("ownerReply", "");

        std::cout << "\n========================================" << std::endl;
        std::cout << "📥 [REQ_REVIEW_REPLY] 사장님 답글 등록 요청 수신" << std::endl;
        std::cout << "👉 storeId: " << storeId << ", reviewId: " << reviewId << std::endl;
        std::cout << "👉 내용: " << ownerReply << std::endl;
        std::cout << "========================================" << std::endl;
        if (reviewId == 0 || storeId == 0 || ownerReply.empty())
        {
            res["status"] = 400;
            res["message"] = "reviewId, storeId, ownerReply 중 누락된 값이 있습니다.";
            std::cout << "📤 [RES_REVIEW_REPLY] 에러 응답 전송:\n"
                      << res.dump(4) << "\n========================================" << std::endl;
            session->sendPacket(static_cast<uint16_t>(CmdID::RES_REVIEW_REPLY), res);
            return;
        }

        auto conn = DBManager::getInstance().getConnection();

        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
            "UPDATE REVIEWS SET owner_reply = ? WHERE review_id = ? AND store_id = ?"));
        pstmt->setString(1, ownerReply);
        pstmt->setInt(2, reviewId);
        pstmt->setInt(3, storeId);

        int affected = pstmt->executeUpdate();

        if (affected > 0)
        {
            res["status"] = 200;
            res["message"] = "답글 등록 완료";
        }
        else
        {
            res["status"] = 404;
            res["message"] = "해당 리뷰를 찾을 수 없거나 권한이 없습니다.";
        }
        std::cout << "📤 [RES_REVIEW_REPLY] 처리 결과 전송:\n"
                  << res.dump(4) << "\n========================================" << std::endl;
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_REVIEW_REPLY), res);
    }
    catch (const std::exception &e)
    {
        std::cerr << "[ReviewHandler] handleReviewReply 오류: " << e.what() << std::endl;
        res["status"] = 500;
        res["message"] = "서버 내부 오류";
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_REVIEW_REPLY), res);
    }
}

// 🚀 [CmdID: 2016] 특정 메뉴에 대한 리뷰만 쏙쏙 골라오기
void ReviewHandler::handleMenuReviewList(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    json req = json::parse(jsonBody);
    json res;
    try
    {
        // 1. 요청받은 menuId 확인
        int menuId = req.value("menuId", 0);

        if (menuId == 0)
        {
            res["status"] = 400;
            res["message"] = "정확한 menuId를 입력해주세요.";
            session->sendPacket(static_cast<uint16_t>(CmdID::RES_MENU_REVIEW_LIST), res);
            return;
        }

        std::cout << "[ReviewHandler] 🍜 메뉴(ID: " << menuId << ") 리뷰 상세 조회 요청" << std::endl;

        auto conn = DBManager::getInstance().getConnection();

        // 2. 쿼리 최적화: REVIEWS 테이블에서 직접 menu_id로 필터링 (JOIN 필요 없음!)
        std::string query = "SELECT review_id, user_id, rating, content, owner_reply, created_at "
                            "FROM REVIEWS "
                            "WHERE menu_id = ? "
                            "ORDER BY created_at DESC";

        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(query));
        pstmt->setInt(1, menuId);

        std::unique_ptr<sql::ResultSet> rs(pstmt->executeQuery());

        json reviews = json::array();
        while (rs->next())
        {
            json rv;
            rv["reviewId"] = rs->getInt("review_id");
            rv["userId"] = std::string(rs->getString("user_id"));
            rv["rating"] = rs->getInt("rating");
            rv["content"] = std::string(rs->getString("content"));
            rv["ownerReply"] = rs->isNull("owner_reply") ? "" : std::string(rs->getString("owner_reply"));
            rv["createdAt"] = std::string(rs->getString("created_at"));
            reviews.push_back(rv);
        }

        // 3. 응답 조립 (아까 성공했던 jina 로그 규격에 맞춤)
        res["status"] = 200;
        res["message"] = "조회 성공";
        res["menuId"] = menuId; // 에코(Echo) 데이터
        res["reviews"] = reviews;

        session->sendPacket(static_cast<uint16_t>(CmdID::RES_MENU_REVIEW_LIST), res);
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [ReviewHandler] handleMenuReviewList 오류: " << e.what() << std::endl;
        res["status"] = 500;
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_MENU_REVIEW_LIST), res);
    }
}