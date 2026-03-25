#pragma once
#include <string>
#include <vector>
#include "json.hpp"

// 👑 1. 관리자 주문 목록 검색 요청 (REQ_ADMIN_ORDER_LIST = 5020)
struct ReqAdminOrderListDTO
{
    std::string keyword;    // 검색어 (없으면 빈 문자열)
    std::string searchType; // "orderId" 또는 "userId"

    // 🚀 검색어 없이 전체 조회를 할 수도 있으므로 WITH_DEFAULT 사용!
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(ReqAdminOrderListDTO, keyword, searchType)
};

// 👑 2. 관리자 화면에 뿌려질 주문 1건의 데이터 (AdminOrderDTO)
struct AdminOrderDTO
{
    std::string orderId;
    std::string userId;
    int totalPrice;
    int orderStatus;
    std::string createdAt;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(AdminOrderDTO, orderId, userId, totalPrice, orderStatus, createdAt)
};

// 👑 3. 관리자 주문 목록 검색 응답 (RES_ADMIN_ORDER_LIST = 5021)
struct ResAdminOrderListDTO
{
    int status;
    std::vector<AdminOrderDTO> orders;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ResAdminOrderListDTO, status, orders)
};

// =========================================================================
// 🎧 [고객센터 1:1 문의 전용 DTO]
// =========================================================================

// 1. 서버 -> 관리자: "고객/사장님이 1:1 문의를 요청했습니다!" (푸시 알림용)
struct NotifyAdminChatReqDTO
{
    std::string requesterId;   // 문의를 요청한 유저 ID (asdf 등)
    std::string requesterType; // "CUSTOMER" 또는 "OWNER" (누가 보냈는지 구분)
    std::string message;       // "새로운 1:1 문의 요청이 들어왔습니다."

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(NotifyAdminChatReqDTO, requesterId, requesterType, message)
};

// 2. 관리자 -> 서버: "그 문의 수락할게. 방 파줘!" (CmdID: 5000)
struct ReqAdminChatAcceptDTO
{
    std::string requesterId; // 수락할 유저의 ID (누구랑 대화할 것인가)
    std::string adminId;     // 수락 버튼을 누른 관리자 본인의 ID

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ReqAdminChatAcceptDTO, requesterId, adminId)
};

// 3. 관리자 -> 서버: "지금 바빠서 문의 거절할래." (CmdID: 5001)
struct ReqAdminChatRejectDTO
{
    std::string requesterId; // 거절할 유저의 ID
    std::string reason;      // 거절 사유 ("현재 모든 상담원이 통화 중입니다. 잠시 후 다시 시도해주세요.")

    // 🚀 사유를 안 적고 닫을 수도 있으니 WITH_DEFAULT로 유연하게 방어!
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(ReqAdminChatRejectDTO, requesterId, reason)
};

// 4. 서버 -> 고객/사장님: "관리자가 수락(또는 거절)했습니다!" (결과 응답용)
struct ResChatRequestResultDTO
{
    int status;          // 200: 수락됨(방 입장), 400: 거절됨
    std::string roomId;  // 수락 시 발급된 채팅방 번호 (거절 시 빈 문자열 "")
    std::string message; // 팝업에 띄울 메시지

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(ResChatRequestResultDTO, status, roomId, message)
};