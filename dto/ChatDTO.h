#include <string>
#include "json.hpp"

// 🙋‍♂️ 2090: 채팅방 입장 요청 (주문 번호 기반)
struct ReqChatConnectDTO
{
    std::string userId; // 채팅 대상 고객 ID
    int storeId;        // 매장 ID

    // 🚀 매크로에서도 꼭 맞춰주세요!
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ReqChatConnectDTO, userId, storeId)
};

// 🙋‍♂️ 2091: 채팅방 입장 응답
struct ResChatConnectDTO
{
    int status; // 0: 성공, 1: 실패
    int roomId; // 발급되거나 이미 존재하는 방 번호
    std::string message;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ResChatConnectDTO, status, roomId, message)
};

// 💬 2092: 메시지 전송 요청
struct ReqChatSendDTO
{
    int roomId;           // 몇 번 방에 보내는가?
    std::string senderId; // 보내는 사람 ID
    std::string content;  // "단무지 많이 주세요!"
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ReqChatSendDTO, roomId, senderId, content)
};

// 💬 2093: 메시지 전송 확인 응답 (보낸 사람에게)
struct ResChatSendDTO
{
    int status;
    std::string message;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ResChatSendDTO, status, message)
};

// 🚀 9030: 상대방에게 메시지 수신 알림 (가장 중요!)
struct NotifyChatRecvDTO
{
    int roomId;
    std::string senderId; // 누가 보냈는지
    std::string content;  // 메시지 내용
    std::string sendTime; // 보낸 시간 (클라이언트에 띄우기 위함)
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(NotifyChatRecvDTO, roomId, senderId, content, sendTime)
};