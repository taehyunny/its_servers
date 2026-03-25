#pragma once
#include <string>
#include <memory>
#include "ClientSession.h"

class ChatHandler
{
public:
    // 🚀 1. 채팅방 입장 (또는 생성) 요청 처리
    static void handleChatConnect(std::shared_ptr<ClientSession> session, const std::string &jsonBody);

    // 🚀 2. 메시지 전송 및 라우팅 처리
    static void handleChatSend(std::shared_ptr<ClientSession> session, const std::string &jsonBody);

private:
    // 🚀 3. (선택) 채팅방 나가기, 채팅
    // 4. (선택) 채팅방 목록 조회 등 추가 기능 핸들러
};
