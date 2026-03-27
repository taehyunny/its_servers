#pragma once
#include <unordered_map>
#include <string>
#include <mutex>
#include <memory>
#include <iostream>
#include "ClientSession.h"

class SessionManager
{
public:
    static SessionManager &getInstance()
    {
        static SessionManager instance;
        return instance;
    }

    void createSession(int fd);
    std::shared_ptr<ClientSession> getSession(int fd);
    std::shared_ptr<ClientSession> getSessionByUserId(const std::string &userId);
    void removeSession(int fd);

    // 1. 로그인 성공 시 유저 등록
    void registerUser(const std::string &userId, int role, std::shared_ptr<ClientSession> session)
    {
        std::lock_guard<std::mutex> lock(sessionMutex);
        session->authenticate(userId, role); // 🚀 세션 객체에 ID와 롤을 확실히 각인!
        userMap[userId] = session;
        std::cout << "[SessionManager] 유저 등록 완료: " << userId << " (Role: " << role << ")" << std::endl;
    }

    // 2. 특정 유저에게 패킷 전송
    template <typename T>
    bool sendToUser(const std::string &userId, uint16_t cmdId, const T &payload)
    {
        std::lock_guard<std::mutex> lock(sessionMutex);
        auto it = userMap.find(userId);

        if (it != userMap.end() && it->second)
        {
            it->second->sendPacket(cmdId, payload);
            return true;
        }
        return false;
    }

    // 3. 특정 역할(Role) 전체에게 브로드캐스트
    template <typename T>
    void broadcastToRole(int targetRole, uint16_t cmdId, const T &payload)
    {
        std::lock_guard<std::mutex> lock(sessionMutex);
        for (auto &pair : userMap)
        {
            if (pair.second && pair.second->getRole() == targetRole)
            {
                pair.second->sendPacket(cmdId, payload);
            }
        }
    }

    void removeUser(const std::string &userId)
    {
        std::lock_guard<std::mutex> lock(sessionMutex);
        userMap.erase(userId);
    }

    // 🚀 관리자 핀셋 호출 함수
    std::shared_ptr<ClientSession> getAvailableAdminSession();

private:
    SessionManager() = default;
    ~SessionManager() = default;

    std::mutex sessionMutex;
    std::unordered_map<int, std::shared_ptr<ClientSession>> sessionMap;      // fd -> 세션
    std::unordered_map<std::string, std::shared_ptr<ClientSession>> userMap; // userId -> 세션
    // 💡 roleMap은 이제 사용하지 않으므로 삭제!
};