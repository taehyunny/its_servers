#pragma once
#include <unordered_map>
#include <string>
#include <mutex>
#include <memory>
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

    // 1. 로그인 시 userId와 세션 연결
    void authenticateUser(const std::string &userId, std::shared_ptr<ClientSession> session)
    {
        std::lock_guard<std::mutex> lock(sessionMutex);
        userMap[userId] = session;
    }

    // 2. [9010번/채팅용] 특정 유저 콕 집어서 알림 쏘기 (템플릿은 헤더에 하나만!)
    template <typename T>
    bool sendToUser(const std::string &userId, uint16_t cmdId, const T &dto)
    {
        std::lock_guard<std::mutex> lock(sessionMutex);
        auto it = userMap.find(userId);

        if (it != userMap.end())
        {
            it->second->sendPacket(cmdId, dto);
            return true; // 전송 성공
        }
        return false; // 오프라인 상태
    }

    void removeUser(const std::string &userId)
    {
        std::lock_guard<std::mutex> lock(sessionMutex);
        userMap.erase(userId);
    }

    // 3. [9020번용] 특정 권한(라이더 등) 전체 브로드캐스트
    template <typename T>
    void broadcastToRole(int targetRole, uint16_t cmdId, const T &dto)
    {
        std::lock_guard<std::mutex> lock(sessionMutex);
        for (auto &pair : userMap)
        {
            if (pair.second->getRole() == targetRole)
            {
                pair.second->sendPacket(cmdId, dto);
            }
        }
    }

private:
    SessionManager() = default;
    ~SessionManager() = default;

    std::mutex sessionMutex;
    std::unordered_map<int, std::shared_ptr<ClientSession>> sessionMap;
    std::unordered_map<std::string, std::shared_ptr<ClientSession>> userMap;
};