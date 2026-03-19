#pragma once
#include <vector>
#include <string>
#include "ClientSession.h"
#include "Global_protocol.h"
#include "ThreadPool.h" // 🚀 ThreadPool 인식용

class PacketFramer
{
public:
    // 🚀 수정: ThreadPool 참조(&) 매개변수 추가
    static void onReceiveData(ClientSession *session, const char *data, int len, ThreadPool &pool);

private:
    static void processBuffer(ClientSession *session, ThreadPool &pool);
};