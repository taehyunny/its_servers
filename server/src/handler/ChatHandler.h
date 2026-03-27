#pragma once
#include <string>
#include <memory>
#include "ClientSession.h"

class ChatHandler
{
public:
    // 🙋‍♂️ 1. 초인종: 사장님(또는 고객)의 채팅 요청
    static void handleChatRequest(std::shared_ptr<ClientSession> session, const std::string &jsonBody);
    // 🚀 2. 메시지 전송 및 라우팅 처리 (기존과 동일)
    static void handleChatSend(std::shared_ptr<ClientSession> session, const std::string &jsonBody);

    // 🧑‍💻 3. 스레드 연결 (5000번): 관리자의 수락/거절 처리 및 방(스레드) 할당
    static void handleChatAccept(std::shared_ptr<ClientSession> session, const std::string &jsonBody);

    // 🚀 4. 채팅 종료 처리 (5002번): 관리자 또는 사장님이 채팅 종료를 요청했을 때
    static void handleChatClose(std::shared_ptr<ClientSession> session, const std::string &jsonBody);
};