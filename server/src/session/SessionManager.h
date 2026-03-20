#pragma once
#include <unordered_map>
#include <mutex>
#include <memory> // shared_ptr을 위해 필수
#include "ClientSession.h"

class SessionManager
{
public:
    void createSession(int fd);
    // 🚀 [수정] 리턴 타입도 무조건 shared_ptr로 맞춰야 합니다!
    std::shared_ptr<ClientSession> getSession(int fd);
    void removeSession(int fd);

private:
    std::mutex sessionMutex; // 🚀 자물쇠 이름 (cpp와 통일)
    // 🚀 [수정] 값의 타입이 shared_ptr이어야 합니다.
    std::unordered_map<int, std::shared_ptr<ClientSession>> sessionMap;
};