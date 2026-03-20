#include "SessionManager.h"
#include <iostream>

void SessionManager::createSession(int fd)
{
    std::lock_guard<std::mutex> lock(sessionMutex); // 🚀 자물쇠!
    // raw pointer 대신 shared_ptr 사용
    sessionMap[fd] = std::make_shared<ClientSession>(fd);
    std::cout << "[SessionManager] 세션 생성 완료 (FD: " << fd << ")" << std::endl;
}

// 🚀 [수정] 반환 타입을 shared_ptr로 변경
std::shared_ptr<ClientSession> SessionManager::getSession(int fd)
{
    std::lock_guard<std::mutex> lock(sessionMutex); // 🚀 조회할 때도 자물쇠 필수!

    auto it = sessionMap.find(fd); // 세션이 존재하는지 먼저 확인
    if (it == sessionMap.end())    // 없으면 nullptr 반환
        return nullptr;

    // 🚀 [수정] sessionMap[fd] 대신 it->second를 반환해야 안전합니다.
    return it->second;
}

void SessionManager::removeSession(int fd) // 세션 제거는 erase만 해도 shared_ptr이 알아서 메모리 정리해줍니다.
{
    std::lock_guard<std::mutex> lock(sessionMutex); // 🚀 자물쇠!
    auto it = sessionMap.find(fd);                  // 세션이 존재하는지 먼저 확인
    if (it != sessionMap.end())                     // 있으면 제거
    {
        // shared_ptr이므로 erase만 해도 참조가 없으면 자동 delete 됩니다.
        sessionMap.erase(it);
        std::cout << "[SessionManager] 세션 제거 완료 (FD: " << fd << ")" << std::endl;
    }
}