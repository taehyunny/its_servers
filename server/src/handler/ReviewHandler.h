#pragma once
#include <string>
#include <memory>

// 전방 선언
class ClientSession;

class ReviewHandler
{
public:
    // 리뷰 목록 조회 (REQ_REVIEW_LIST = 2014)
    static void handleReviewList(std::shared_ptr<ClientSession> session, const std::string &jsonBody);
    // 특정 메뉴 리뷰 목록 조회 (REQ_MENU_REVIEW_LIST = 2016)
    static void handleMenuReviewList(std::shared_ptr<ClientSession> session, const std::string &jsonBody);
    // 답글 등록 (REQ_REVIEW_REPLY = 3070)
    static void handleReviewReply(std::shared_ptr<ClientSession> session, const std::string &jsonBody);
};