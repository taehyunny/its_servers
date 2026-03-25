#pragma once
#include <string>
#include <memory>
#include "ClientSession.h"

class ChatHandler
{
public:
    // 🚀 1. 채팅방 연결 요청 (방 생성 X, 사장님께 푸시만 전송)
    static void handleChatConnect(std::shared_ptr<ClientSession> session, const std::string &jsonBody);

    // 🚀 2. 메시지 전송 및 라우팅 처리 (기존과 동일)
    static void handleChatSend(std::shared_ptr<ClientSession> session, const std::string &jsonBody);

    // 🚀 3. [신규] 사장님/관리자의 1:1 문의 수락 (5000)
    static void handleAdminChatAccept(std::shared_ptr<ClientSession> session, const std::string &jsonBody);

    // 🚀 4. [신규] 사장님/관리자의 1:1 문의 거절 (5001)
    static void handleAdminChatReject(std::shared_ptr<ClientSession> session, const std::string &jsonBody);
};