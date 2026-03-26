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

    // 1. 로그인 성공 시 유저 등록 (기존 authenticateUser와 동일 기능을 registerUser로 통일)
    void registerUser(const std::string &userId, std::shared_ptr<ClientSession> session)
    {
        std::lock_guard<std::mutex> lock(sessionMutex);
        userMap[userId] = session;
        std::cout << "[SessionManager] 유저 등록 완료: " << userId << std::endl;
    }

    // 2. 특정 유저에게 패킷 전송 (템플릿 하나로 DTO와 json 모두 처리 가능!)
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

private:
    SessionManager() = default;
    ~SessionManager() = default;

    std::mutex sessionMutex;
    std::unordered_map<int, std::shared_ptr<ClientSession>> sessionMap;
    std::unordered_map<std::string, std::shared_ptr<ClientSession>> userMap;
};