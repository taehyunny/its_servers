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
    void registerUser(const std::string &userId, int role, std::shared_ptr<ClientSession> session)
    {
        std::lock_guard<std::mutex> lock(sessionMutex);
        session->authenticate(userId, role); // 🚀 세션 객체에 ID와 롤을 확실히 각인!
        userMap[userId] = session;
        std::cout << "[SessionManager] 유저 등록 완료: " << userId << " (Role: " << role << ")" << std::endl;
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
    // [SessionManager.h 내부] 클래스 안에 이 함수를 추가해 주세요!
    std::shared_ptr<ClientSession> getAvailableAdminSession()
    {
        std::lock_guard<std::mutex> lock(sessionMutex);
        
        int ADMIN_ROLE = 3; // 💡 실제 DB의 관리자 Role 번호(예: 3)에 맞게 수정하세요.

        for (auto &pair : userMap)
        {
            // 세션이 살아있고, 역할이 관리자인 첫 번째 사람을 찾으면 즉시 반환!
            if (pair.second && pair.second->getRole() == ADMIN_ROLE)
            {
                std::cout << "[SessionManager] 🎧 대기 중인 관리자(" << pair.first << ") 배정 완료!" << std::endl;
                return pair.second;
            }
        }
        
        return nullptr; // 접속 중인 관리자가 아무도 없을 때
    }

private:
    SessionManager() = default;
    ~SessionManager() = default;

    std::mutex sessionMutex;
    std::unordered_map<int, std::shared_ptr<ClientSession>> sessionMap;  // fd -> 세션
    std::unordered_map<std::string, std::shared_ptr<ClientSession>> userMap; // userId -> 세션
    std::unordered_map<int, std::vector<std::shared_ptr<ClientSession>>> roleMap;  // role -> 세션 리스트 (필요 시 구현)
};std::unordered_map<int, std::vector<std::shared_ptr<ClientSession>>> roleMap;