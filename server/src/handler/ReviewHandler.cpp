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
void ReviewHandler::handleReviewList(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    json req = json::parse(jsonBody);
    json res;
    try
    {
        int storeId = req.value("storeId", 0);
        if (storeId == 0)
        {
            res["status"] = 400;
            res["message"] = "storeId가 없습니다.";
            session->sendPacket(static_cast<uint16_t>(CmdID::RES_REVIEW_LIST), res);
            return;
        }

        auto conn = DBManager::getInstance().getConnection();

        std::unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
            "SELECT review_id, user_id, rating, content, owner_reply, created_at "
            "FROM REVIEWS WHERE store_id = ? ORDER BY created_at DESC"));
        pstmt->setInt(1, storeId);
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
        res["reviews"] = reviews;

        session->sendPacket(static_cast<uint16_t>(CmdID::RES_REVIEW_LIST), res);
    }
    catch (const std::exception &e)
    {
        std::cerr << "[ReviewHandler] handleReviewList 오류: " << e.what() << std::endl;
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

        if (reviewId == 0 || storeId == 0 || ownerReply.empty())
        {
            res["status"] = 400;
            res["message"] = "reviewId, storeId, ownerReply 중 누락된 값이 있습니다.";
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