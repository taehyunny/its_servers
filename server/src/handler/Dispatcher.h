#pragma once

#include <unordered_map>
#include <functional>
#include <memory>
#include <string>
#include "Global_protocol.h"
#include "ClientSession.h"
#include "thread/ThreadPool.h" // ThreadPool 인식용
#include "AllDTOs.h"
#include <string>

class Dispatcher {
private:
    // 공통된 핸들러 함수의 형태를 정의합니다.
    using HandlerFunc = std::function<void(std::shared_ptr<ClientSession>, const std::string&)>;
    
    // 명령어를 키(Key)로, 함수를 값(Value)으로 가지는 맵
    static const std::unordered_map<CmdID, HandlerFunc> _handlerMap;

public:
    static void dispatch(std::shared_ptr<ClientSession> session, const PacketHeader &header, const std::string &jsonBody, ThreadPool &pool);
};