#include "SessionManager.h"
#include "ClientSession.h"
#include <iostream>
#include <algorithm>
#include <unistd.h> // close() 함수 사용을 위해 추가
#include <mutex>    // std::lock_guard 사용을 위해 추가

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

// 🚀 roleMap 대신 userMap을 순회하여 관리자를 찾는 완벽한 로직!
std::shared_ptr<ClientSession> SessionManager::getAvailableAdminSession()
{
    std::lock_guard<std::mutex> lock(sessionMutex);

    int ADMIN_ROLE = 3; // 💡 관리자 롤 번호 (필요시 맞게 수정하세요)

    for (auto &pair : userMap)
    {
        // 세션이 살아있고, 해당 세션의 역할(Role)이 3이라면 즉시 반환!
        if (pair.second && pair.second->getRole() == ADMIN_ROLE)
        {
            std::cout << "[SessionManager] 🎧 대기 중인 관리자(" << pair.first << ") 배정 완료!" << std::endl;
            return pair.second;
        }
    }

    return nullptr; // 접속 중인 관리자가 없음!
}