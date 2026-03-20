#pragma once

#include "Global_protocol.h"
#include "ClientSession.h"
#include "thread/ThreadPool.h" // ThreadPool 인식용
#include <string>

class Dispatcher
{
public:
    // 🚀 수정: ThreadPool 참조(&)를 매개변수로 추가로 받습니다.
    static void dispatch(std::shared_ptr<ClientSession> session, const PacketHeader &header, const std::string &jsonBody, ThreadPool &pool);

private:
    Dispatcher() = delete;
};