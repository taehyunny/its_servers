#include "ChatHandler.h"
#include "ChatRoomManager.h"
#include "SessionManager.h"
#include "AllDTOs.h" // 🚀 태현님이 올려주신 DTO들이 있는 헤더
#include "Global_protocol.h"
#include "ClientSession.h"
#include <iostream>
#include <nlohmann/json.hpp>
#include <ctime>

// =========================================================
// 🙋‍♂️ 1. 초인종: 고객(Role 0) & 사장님(Role 1)의 채팅 요청 (CmdID: 2090)
// =========================================================
void ChatHandler::handleChatRequest(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        auto req = nlohmann::json::parse(jsonBody).get<ReqChatConnectDTO>();

        // 🚀 1. 핵심 해결책: 프론트가 보낸 req.userId는 믿지 말고 버립니다!
        // 대신 서버에 로그인할 때 발급된 "진짜 세션 ID"를 강제로 꺼내옵니다.
        std::string actualUserId = session->getUserId();

        if (actualUserId.empty())
        {
            throw std::runtime_error("로그인되지 않은 세션입니다.");
        }

        int role = session->getRole();
        std::string roleType = (role == 1) ? "STORE_OWNER" : "CUSTOMER";
        std::string roleName = (role == 1) ? "사장님" : "고객님";

        // 🚀 2. 관리자에게 보낼 알림 (req.userId가 아닌 actualUserId를 탑재!)
        nlohmann::json adminNotify = {
            {"userId", actualUserId}, // 💡 여기서 진짜 ID가 관리자에게 넘어갑니다!
            {"storeId", req.storeId},
            {"role", role},
            {"message", roleName + " [" + actualUserId + "] 님의 1:1 채팅 요청"}};

        SessionManager::getInstance().broadcastToRole(
            3,
            static_cast<uint16_t>(CmdID::NOTIFY_NEW_CHAT_REQUEST), // 9030
            adminNotify);

        ResChatConnectDTO res = {0, -1, "상담원 연결을 대기 중입니다..."};
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_CHAT_CONNECT), nlohmann::json(res));

        std::cout << "[ChatHandler] 🔔 " << roleName << "(" << actualUserId << ") 요청 관리자에게 전송 완료" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [ChatHandler] 채팅 요청 처리 에러: " << e.what() << std::endl;
    }
}
// =========================================================
// 🧑‍💻 2. 스레드 연결 (5000번): 관리자의 수락 처리 및 방 할당
// =========================================================
void ChatHandler::handleChatAccept(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        // 🚀 순정 ReqChatAcceptDTO 사용! (requesterId, requesterType, result 포함)
        auto req = nlohmann::json::parse(jsonBody).get<ReqChatAcceptDTO>();
        std::string adminId = session->getUserId();
        if (adminId.empty())
            adminId = "admin";

        std::cout << "[ChatHandler] 🧑‍💻 관리자 수락: " << req.requesterId << " (" << req.requesterType << ")" << std::endl;

        if (req.result == "ACCEPT")
        {
            // 방 생성
            int roomId = ChatRoomManager::getInstance().createRoom(req.requesterId, adminId, 0);

            // 관리자에게 방 번호 전달
            ResChatConnectDTO adminRes = {0, roomId, "채팅방 생성 완료"};
            session->sendPacket(static_cast<uint16_t>(CmdID::RES_REQUEST_OK), nlohmann::json(adminRes));

            // 🚀 고객/사장님에게 수락 알림 전송 (9032번)
            nlohmann::json requesterNotify = {
                {"roomId", roomId},
                {"adminId", adminId},
                {"message", "상담원이 연결되었습니다."}};
            SessionManager::getInstance().sendToUser(req.requesterId, static_cast<uint16_t>(CmdID::NOTIFY_CHAT_ROOM_OPENED), requesterNotify);

            std::cout << "[ChatHandler] ✅ [Room: " << roomId << "] 중계 채널 오픈 완료!" << std::endl;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [ChatHandler] 수락 처리 에러: " << e.what() << std::endl;
    }
}

// =========================================================
// 💬 3. 채팅 중계 (라우팅) (CmdID: 2092)
// =========================================================
void ChatHandler::handleChatSend(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        // 🚀 순정 ReqChatSendDTO 사용!
        auto req = nlohmann::json::parse(jsonBody).get<ReqChatSendDTO>();
        auto room = ChatRoomManager::getInstance().getRoom(req.roomId);
        if (!room)
            throw std::runtime_error("존재하지 않는 방입니다.");

        std::string targetUserId = (req.senderId == room->adminId) ? room->customerId : room->adminId;

        time_t now = time(nullptr);
        char timeBuf[10];
        strftime(timeBuf, sizeof(timeBuf), "%H:%M", localtime(&now));

        // 🚀 순정 NotifyChatRecvDTO 그릇 사용!
        NotifyChatRecvDTO notify = {req.roomId, req.senderId, req.content, std::string(timeBuf)};

        // 🚀 상대방이 관리자면 9031, 고객/사장님이면 9030
        uint16_t notifyCmdId = (targetUserId == room->adminId) ? static_cast<uint16_t>(CmdID::NOTIFY_MSG_TO_ADMIN) : static_cast<uint16_t>(CmdID::NOTIFY_MSG_TO_USER);

        SessionManager::getInstance().sendToUser(targetUserId, notifyCmdId, nlohmann::json(notify));

        // 🚀 순정 ResChatSendDTO 사용!
        ResChatSendDTO res = {200, "전송 완료"};
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_CHAT_SEND), nlohmann::json(res));
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [ChatHandler] 메시지 전송 에러: " << e.what() << std::endl;
    }
}