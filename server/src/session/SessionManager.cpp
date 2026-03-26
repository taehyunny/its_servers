#include "SessionManager.h"
#include "ClientSession.h"
#include <iostream>

void SessionManager::createSession(int fd)
{
    std::lock_guard<std::mutex> lock(sessionMutex);
    sessionMap[fd] = std::make_shared<ClientSession>(fd);
    std::cout << "[SessionManager] 세션 생성 완료 (FD: " << fd << ")" << std::endl;
}

std::shared_ptr<ClientSession> SessionManager::getSession(int fd)
{
    std::lock_guard<std::mutex> lock(sessionMutex);

    auto it = sessionMap.find(fd);
    if (it == sessionMap.end())
        return nullptr;

    return it->second;
}

void SessionManager::removeSession(int fd)
{
    std::lock_guard<std::mutex> lock(sessionMutex);

    auto it = sessionMap.find(fd);
    if (it != sessionMap.end())
    {
        std::shared_ptr<ClientSession> session = it->second;

        std::string userId = session->getUserId();
        if (!userId.empty())
        {
            userMap.erase(userId);
            std::cout << "[SessionManager] 유저 '" << userId << "'의 논리적 연결 해제 (fd: " << fd << ")" << std::endl;
        }

        sessionMap.erase(it);
        std::cout << "[SessionManager] 소켓 " << fd << " 물리적 세션 제거 완료." << std::endl;
    }
}

std::shared_ptr<ClientSession> SessionManager::getSessionByUserId(const std::string &userId)
{
    std::lock_guard<std::mutex> lock(sessionMutex);

    auto it = userMap.find(userId);
    if (it != userMap.end())
    {
        return it->second;
    }

    return nullptr;
}

// ===================================================================================
// 🚀 [여기서부터 신규 로직] 아키텍처의 꽃: 권한/유저 라우팅 기능 구현부
// ===================================================================================

// 1. 유저 맵 등록 (로그인이나 더미 인증 성공 시 호출해야 함!)
void SessionManager::registerUser(const std::string &userId, std::shared_ptr<ClientSession> session)
{
    std::lock_guard<std::mutex> lock(sessionMutex);
    userMap[userId] = session;
    std::cout << "[SessionManager] 유저 '" << userId << "' 맵핑 완료 (FD: " << session->getFd() << ")" << std::endl;
}

// 2. 특정 유저에게 전송 (1:1 채팅 9030 등에서 사용)
bool SessionManager::sendToUser(const std::string &userId, uint16_t cmdId, const nlohmann::json &payload)
{
    // 자물쇠는 getSessionByUserId 내부에서 이미 채우므로 여기서 안 채워도 안전!
    auto session = getSessionByUserId(userId);
    if (session)
    {
        session->sendPacket(cmdId, payload);
        return true;
    }
    return false; // 오프라인
}

// 3. 🚀 특정 권한(Role) 그룹 전체에 브로드캐스트 (CS센터 관리자 9040 호출용!)
bool SessionManager::broadcastToRole(int targetRole, uint16_t cmdId, const nlohmann::json &payload)
{
    std::lock_guard<std::mutex> lock(sessionMutex); // 맵 전체를 훑어야 하므로 자물쇠 필수
    bool isSentToAnyone = false;

    // 접속 중인 모든 세션(sessionMap)을 훑어봄
    for (const auto &pair : sessionMap)
    {
        auto session = pair.second;
        // 세션이 살아있고, 명찰(Role)이 타겟 권한(ex: 3)과 일치한다면 쏜다!
        if (session && session->getRole() == targetRole)
        {
            session->sendPacket(cmdId, payload);
            isSentToAnyone = true;
        }
    }

    return isSentToAnyone; // 한 명이라도 받았다면 true 반환
}