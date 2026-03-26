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


std::shared_ptr<ClientSession> SessionManager::getAvailableAdminSession()
{
    std::lock_guard<std::mutex> lock(sessionMutex);
    
    int ADMIN_ROLE = 3; // 관리자 롤 번호 (가정)
    
    auto it = roleMap.find(ADMIN_ROLE);
    if (it != roleMap.end() && !it->second.empty()) {
        // 🚀 접속 중인 관리자가 있다면, 첫 번째 관리자 세션 반환 (또는 랜덤 배정)
        return it->second.front(); 
    }
    
    return nullptr; // 접속 중인 관리자가 없음!
}