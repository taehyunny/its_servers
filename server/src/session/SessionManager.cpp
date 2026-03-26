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


