#pragma once
#include <unordered_map>
#include <string>
#include <mutex>
#include <memory>
#include "ClientSession.h"

class SessionManager
{
public:
    // 🚀 전역에서 SessionManager를 부르기 위한 싱글톤!
    static SessionManager &getInstance()
    {
        static SessionManager instance;
        return instance;
    }

    // [기존 물리적 관리] 소켓(fd) 기반 연결/해제
    void createSession(int fd);
    std::shared_ptr<ClientSession> getSession(int fd);
    // 🚀 유저 ID로 특정 세션 찾기 (사장님 푸시 알림용)
    std::shared_ptr<ClientSession> getSessionByUserId(const std::string &userId);
    void removeSession(int fd); // 🚨 주의: 여기서 userMap의 데이터도 같이 지워야 합니다!

    // ==========================================================
    // 🚀 [신규 논리적 관리] 실시간 브로드캐스팅(Push)을 위한 무기들
    // ==========================================================

    // 1. 로그인 성공 시, 아이디(userId)와 소켓(Session)을 연결해주는 함수
    void authenticateUser(const std::string &userId, std::shared_ptr<ClientSession> session)
    {
        std::lock_guard<std::mutex> lock(sessionMutex);
        userMap[userId] = session; // "jina는 3번 소켓이다" 라고 기록!
    }

    // 2. [9010번용] 특정 유저(고객)를 콕 집어서 알림 쏘기
    template <typename T>
    void sendToUser(const std::string &userId, uint16_t cmdId, const T &dto)
    {
        std::lock_guard<std::mutex> lock(sessionMutex);
        auto it = userMap.find(userId);
        if (it != userMap.end())
        {
            it->second->sendPacket(cmdId, dto); // jina의 소켓을 찾아서 발사!
        }
    }
    void removeUser(const std::string &userId)
    {
        std::lock_guard<std::mutex> lock(sessionMutex);
        userMap.erase(userId); // 전화번호부에서 이름만 지우기!
    }

    // 3. [9020번용] 특정 권한(라이더) 전체에게 브로드캐스팅 쏘기
    template <typename T>
    void broadcastToRole(int targetRole, uint16_t cmdId, const T &dto)
    {
        std::lock_guard<std::mutex> lock(sessionMutex);
        for (auto &pair : userMap)
        {
            // (ClientSession에 getRole() 함수가 있다고 가정)
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

    // 물리적 소켓 연결망 (네트워크 끊김 대비용)
    std::unordered_map<int, std::shared_ptr<ClientSession>> sessionMap;

    // 논리적 아이디 연결망 (알림 쏠 때 이름으로 찾기용)
    std::unordered_map<std::string, std::shared_ptr<ClientSession>> userMap;
};
