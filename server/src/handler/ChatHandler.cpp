#include "ChatHandler.h"
#include "ChatRoomManager.h"
#include "SessionManager.h"
#include "OrderDAO.h"
#include "StoreDAO.h"
#include "AllDTOs.h"
#include <iostream>
#include <chrono>
#include <iomanip>

// 🚀 1. 채팅방 입장 (또는 생성) 요청 처리 (2090)
void ChatHandler::handleChatConnect(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        auto req = nlohmann::json::parse(jsonBody).get<ReqChatConnectDTO>();
        std::cout << "[ChatHandler] 🚪 채팅방 입장 요청 (Store: " << req.storeId << ", User: " << req.userId << ")" << std::endl;

        // 1. DTO에서 바로 값을 빼옵니다.
        std::string customerId = req.userId;

        // 2. StoreDAO를 통해 매장 사장님 ID를 찾습니다.
        std::string adminId = StoreDAO::getInstance().getOwnerIdByStoreId(req.storeId);

        if (adminId.empty())
        {
            throw std::runtime_error("매장 사장님 정보를 찾을 수 없습니다.");
        }

        // 3. ChatRoomManager를 통해 방을 생성합니다.
        // 💡 MVP 버전이므로 orderId는 0(또는 임의의 값)으로 넘겨버립니다!
        int roomId = ChatRoomManager::getInstance().createRoom(customerId, adminId, 0);

        // 4. 성공 응답 전송 (2091)
        ResChatConnectDTO res = {0, roomId, "채팅방이 연결되었습니다."};
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_CHAT_CONNECT), nlohmann::json(res));
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [ChatHandler] 채팅방 연결 에러: " << e.what() << std::endl;
        ResChatConnectDTO res = {1, -1, "채팅방 연결 실패"};
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_CHAT_CONNECT), nlohmann::json(res));
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