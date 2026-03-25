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
        std::cout << "[ChatHandler] 🚪 채팅방 입장 요청 (Store: " << req.storeId << ", User: " << req.userId << ")" << std::endl;

        std::string customerId = req.userId;

        // StoreDAO를 통해 매장 사장님 ID를 찾습니다.
        std::string adminId = StoreDAO::getInstance().getOwnerIdByStoreId(req.storeId);

        if (adminId.empty())
        {
            throw std::runtime_error("매장 사장님 정보를 찾을 수 없습니다.");
        }

        // ❌ [기존 코드 삭제]: 즉시 방을 생성하던 로직 삭제!
        // int roomId = ChatRoomManager::getInstance().createRoom(customerId, adminId, 0);

        // 🚀 [수정 로직]: 사장님(adminId)에게 푸시 알림만 쏩니다!
        NotifyAdminChatReqDTO notifyAdmin = {customerId, "CUSTOMER", customerId + " 고객님이 1:1 문의를 요청했습니다."};

        bool isPushed = SessionManager::getInstance().sendToUser(
            adminId,
            static_cast<uint16_t>(9030), // 💡 임의의 알림 패킷 번호 (예: CmdID::NOTIFY_ADMIN_CHAT_REQ)
            nlohmann::json(notifyAdmin));

        // 🚀 고객에게는 "수락 대기 중"이라는 202 응답을 전송
        int status = isPushed ? 202 : 404;
        std::string msg = isPushed ? "사장님의 연결을 기다리고 있습니다..." : "현재 사장님이 오프라인 상태입니다.";

        ResChatConnectDTO res = {status, -1, msg};
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_CHAT_CONNECT), nlohmann::json(res));
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [ChatHandler] 채팅방 연결 에러: " << e.what() << std::endl;
        ResChatConnectDTO res = {1, -1, "채팅방 연결 실패"};
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_CHAT_CONNECT), nlohmann::json(res));
    }
}

// 🚀 2. 2단계 (A): 사장님의 수락 (5000) -> 찐 방 생성!
void ChatHandler::handleAdminChatAccept(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        auto req = nlohmann::json::parse(jsonBody).get<ReqAdminChatAcceptDTO>();
        std::cout << "[ChatHandler] ✅ 사장님(" << req.adminId << ") 문의 수락! (대상: " << req.requesterId << ")" << std::endl;

        // 🚀 여기서 드디어 방을 생성합니다! (MVP 버전이므로 orderId는 0)
        int roomId = ChatRoomManager::getInstance().createRoom(req.requesterId, req.adminId, 0);

        // 성공 결과 DTO 조립 (앞서 만든 ResChatRequestResultDTO 활용)
        ResChatRequestResultDTO resultDto = {200, std::to_string(roomId), "사장님과 연결되었습니다!"};

        // 1. 수락을 누른 사장님에게 응답
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_REQUEST_OK), nlohmann::json(resultDto));

        // 2. 기다리던 고객에게 응답 (이 패킷을 받으면 로딩창이 닫히고 채팅창이 열림)
        SessionManager::getInstance().sendToUser(
            req.requesterId,
            static_cast<uint16_t>(CmdID::RES_REQUEST_OK),
            nlohmann::json(resultDto));
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
        std::cout << "[ChatHandler] 🚫 사장님 문의 거절 (대상: " << req.requesterId << ")" << std::endl;

        // 실패 결과 DTO 조립
        ResChatRequestResultDTO resultDto = {400, "", req.reason.empty() ? "현재 사장님이 바빠 연결할 수 없습니다." : req.reason};

        // 1. 거절을 누른 사장님에게 완료 응답
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_REQUEST_NO), nlohmann::json(resultDto));

        // 2. 기다리던 고객에게 거절 메시지 전송 (로딩창 닫고 알림 띄우기)
        SessionManager::getInstance().sendToUser(
            req.requesterId,
            static_cast<uint16_t>(CmdID::RES_REQUEST_NO),
            nlohmann::json(resultDto));
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [ChatHandler] 거절 처리 에러: " << e.what() << std::endl;
    }
}

// 🚀 2. 메시지 전송 및 라우팅 처리 (2092 -> 9030)
void ChatHandler::handleChatSend(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        auto req = nlohmann::json::parse(jsonBody).get<ReqChatSendDTO>();

        // 1. 매니저에게 방 정보를 물어봅니다.
        auto room = ChatRoomManager::getInstance().getRoom(req.roomId);
        if (!room)
        {
            throw std::runtime_error("존재하지 않는 채팅방입니다.");
        }

        // 2. 🚀 [사유하는 라우팅 로직] 내가 누군지 파악하고, 상대방 타겟팅!
        std::string receiverId = (req.senderId == room->customerId) ? room->adminId : room->customerId;

        // 3. 현재 시간 구하기 (예: "15:30")
        auto now = std::chrono::system_clock::now();
        time_t time_now = std::chrono::system_clock::to_time_t(now);
        char timeBuf[10];
        strftime(timeBuf, sizeof(timeBuf), "%H:%M", localtime(&time_now));

        // 4. 상대방에게 푸시 알림 쏘기 (9030)
        NotifyChatRecvDTO notify = {req.roomId, req.senderId, req.content, std::string(timeBuf)};

        // 💡 핵심: 실제로 전송을 시도하고 그 결과(성공/실패)를 변수에 담습니다!
        bool isSent = SessionManager::getInstance().sendToUser(
            receiverId,
            static_cast<uint16_t>(CmdID::NOTIFY_CHAT_MSG),
            nlohmann::json(notify));

        // 🚀 5. 전송 결과에 따른 분기 처리 (방어 로직)
        if (isSent)
        {
            // 상대방에게 전달 성공했을 때
            ResChatSendDTO res = {0, "전송 성공"};
            session->sendPacket(static_cast<uint16_t>(CmdID::RES_CHAT_SEND), nlohmann::json(res));
            std::cout << "[ChatHandler] ✅ 메시지 전달 완료: " << req.senderId << " -> " << receiverId << std::endl;
        }
        else
        {
            // 상대방이 오프라인이거나 세션이 없을 때
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