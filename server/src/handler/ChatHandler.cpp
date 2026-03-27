#include "ChatHandler.h"
#include "ChatRoomManager.h"
#include "SessionManager.h"
#include "OrderDAO.h"
#include "StoreDAO.h"
#include "AllDTOs.h"
#include <iostream>
#include <chrono>
#include <iomanip>

// 🚀 1. 1단계: 고객의 똑똑똑 (채팅방 연결 요청 - 2090)
void ChatHandler::handleChatConnect(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        auto req = nlohmann::json::parse(jsonBody).get<ReqChatConnectDTO>();
        std::string customerId = req.userId;

        // 🚀 1. 사장님이 아니라, '접속 중인 관리자(Role 3)'를 찾습니다!
        std::string adminId = "";
        auto adminSession = SessionManager::getInstance().getAvailableAdminSession();

        if (adminSession)
        {
            adminId = adminSession->getUserId();
            std::cout << " >>> [DEBUG] 대기 중인 관리자(Role 3) 발견: [" << adminId << "]" << std::endl;
        }
        else
        {
            // 🚨 접속 중인 관리자가 한 명도 없을 때
            ResChatConnectDTO res = {404, -1, "현재 상담 가능한 관리자가 모두 오프라인입니다."};
            session->sendPacket(static_cast<uint16_t>(CmdID::RES_CHAT_CONNECT), nlohmann::json(res));
            return;
        }

        // 🚀 2. 관리자에게 9030(또는 약속된 번호)으로 상담 요청 푸시 발송
        NotifyAdminChatReqDTO notifyAdmin = {customerId, "CUSTOMER", customerId + " 고객님이 1:1 상담을 요청했습니다."};
        uint16_t PUSH_CMD_ID = static_cast<uint16_t>(CmdID::NOTIFY_ADMIN_CHAT_REQ);

        bool isPushed = SessionManager::getInstance().sendToUser(adminId, PUSH_CMD_ID, nlohmann::json(notifyAdmin));

        // 고객에게는 상태에 따라 202(대기) 또는 404(오프라인) 응답 전송
        int status = isPushed ? 202 : 404;
        std::string msg = isPushed ? "상담원의 연결을 기다리고 있습니다..." : "현재 상담원이 오프라인 상태입니다.";

        ResChatConnectDTO res = {status, -1, msg};
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_CHAT_CONNECT), nlohmann::json(res));
        std::cout << "==================================================\n"
                  << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [ChatHandler] 채팅방 연결 에러: " << e.what() << std::endl;
        ResChatConnectDTO res = {1, -1, "채팅방 연결 실패: " + std::string(e.what())};
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_CHAT_CONNECT), nlohmann::json(res));
    }
}

// 🚀 2. 2단계 (A): 사장님의 수락 (5000) -> 찐 방 생성!
void ChatHandler::handleAdminChatAccept(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        auto req = nlohmann::json::parse(jsonBody).get<ReqAdminChatAcceptDTO>();
        std::cout << "\n==================================================" << std::endl;
        std::cout << "[ChatHandler] ✅ 사장님 문의 수락 패킷 수신 (Cmd: 5000)" << std::endl;
        std::cout << " >>> [DEBUG] 수락한 사장님 ID: " << req.adminId << std::endl;
        std::cout << " >>> [DEBUG] 대기 중인 고객 ID: " << req.requesterId << std::endl;

        // 방 생성
        int roomId = ChatRoomManager::getInstance().createRoom(req.requesterId, req.adminId, 0);
        std::cout << " >>> [DEBUG] 채팅방 생성 완료! (RoomID: " << roomId << ")" << std::endl;

        ResChatRequestResultDTO resultDto = {200, std::to_string(roomId), "사장님과 연결되었습니다!"};

        // 1. 수락을 누른 사장님에게 응답 (채팅창 열기)
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_REQUEST_OK), nlohmann::json(resultDto));

        // 2. 기다리던 고객에게 응답 (로딩창 닫고 채팅창 열기)
        bool isSentToCustomer = SessionManager::getInstance().sendToUser(
            req.requesterId,
            static_cast<uint16_t>(CmdID::RES_REQUEST_OK),
            nlohmann::json(resultDto));

        if (isSentToCustomer)
        {
            std::cout << " >>> [DEBUG] 고객(" << req.requesterId << ")에게 수락 결과 전달 성공!" << std::endl;
        }
        else
        {
            std::cerr << " >>> [DEBUG] 🚨 고객에게 결과 전달 실패! (고객이 기다리다 나갔을 수 있습니다)" << std::endl;
        }
        std::cout << "==================================================\n"
                  << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [ChatHandler] 수락 처리 에러: " << e.what() << std::endl;
    }
}

// 🚀 3. 2단계 (B): 사장님의 거절 (5001)
void ChatHandler::handleAdminChatReject(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        auto req = nlohmann::json::parse(jsonBody).get<ReqAdminChatRejectDTO>();
        std::cout << "\n==================================================" << std::endl;
        std::cout << "[ChatHandler] 🚫 사장님 문의 거절 패킷 수신 (Cmd: 5001)" << std::endl;
        std::cout << " >>> [DEBUG] 거절 대상 고객 ID: " << req.requesterId << std::endl;
        std::cout << " >>> [DEBUG] 거절 사유: " << req.reason << std::endl;

        ResChatRequestResultDTO resultDto = {400, "", req.reason.empty() ? "현재 사장님이 바빠 연결할 수 없습니다." : req.reason};

        // 1. 거절을 누른 사장님에게 완료 응답
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_REQUEST_NO), nlohmann::json(resultDto));

        // 2. 고객에게 거절 메시지 전송
        SessionManager::getInstance().sendToUser(
            req.requesterId,
            static_cast<uint16_t>(CmdID::RES_REQUEST_NO),
            nlohmann::json(resultDto));

        std::cout << " >>> [DEBUG] 거절 처리 완료 및 고객에게 알림 전송됨." << std::endl;
        std::cout << "==================================================\n"
                  << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [ChatHandler] 거절 처리 에러: " << e.what() << std::endl;
    }
}

// 🚀 4. 메시지 전송 및 라우팅 (2092)
void ChatHandler::handleChatSend(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        auto req = nlohmann::json::parse(jsonBody).get<ReqChatSendDTO>();

        auto room = ChatRoomManager::getInstance().getRoom(req.roomId);
        if (!room)
        {
            throw std::runtime_error("존재하지 않는 채팅방입니다.");
        }

        std::string receiverId = (req.senderId == room->customerId) ? room->adminId : room->customerId;

        auto now = std::chrono::system_clock::now();
        time_t time_now = std::chrono::system_clock::to_time_t(now);
        char timeBuf[10];
        strftime(timeBuf, sizeof(timeBuf), "%H:%M", localtime(&time_now));

        NotifyChatRecvDTO notify = {req.roomId, req.senderId, req.content, std::string(timeBuf)};

        bool isSent = SessionManager::getInstance().sendToUser(
            receiverId,
            static_cast<uint16_t>(CmdID::NOTIFY_CHAT_MSG),
            nlohmann::json(notify));

        if (isSent)
        {
            ResChatSendDTO res = {0, "전송 성공"};
            session->sendPacket(static_cast<uint16_t>(CmdID::RES_CHAT_SEND), nlohmann::json(res));
            // 너무 잦은 메시지 로그는 콘솔을 어지럽히므로, 필요시 주석 해제하세요.
            // std::cout << "[ChatHandler] 💬 메시지 전달: " << req.senderId << " -> " << receiverId << std::endl;
        }
        else
        {
            ResChatSendDTO res = {1, "상대방이 오프라인 상태입니다."};
            session->sendPacket(static_cast<uint16_t>(CmdID::RES_CHAT_SEND), nlohmann::json(res));
            std::cerr << "🚨 [ChatHandler] 메시지 전달 실패 (오프라인): 타겟 " << receiverId << std::endl;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [ChatHandler] 메시지 처리 에러: " << e.what() << std::endl;
        ResChatSendDTO res = {1, "메시지 처리 중 오류가 발생했습니다."};
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_CHAT_SEND), nlohmann::json(res));
    }
}