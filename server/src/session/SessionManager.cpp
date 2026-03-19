// 서버에 접속한 수천 명의 ClientSession을 총괄 관리하는 지휘통제실입니다.
#include "SessionManager.h"
#include <iostream>

void SessionManager::createSession(int fd)
{
    // 1만 명 이상의 동시 접속을 고려한 효율적인 세션 생성
    sessionMap[fd] = new ClientSession(fd);
    std::cout << "[SessionManager] 세션 생성 완료 (FD: " << fd << ")" << std::endl;
}

ClientSession *SessionManager::getSession(int fd)
{
    if (sessionMap.find(fd) == sessionMap.end())
        return nullptr;
    return sessionMap[fd];
}

void SessionManager::removeSession(int fd)
{
    auto it = sessionMap.find(fd);
    if (it != sessionMap.end())
    {
        delete it->second; // 메모리 누수 방지
        sessionMap.erase(it);
        std::cout << "[SessionManager] 세션 제거 완료 (FD: " << fd << ")" << std::endl;
    }
}