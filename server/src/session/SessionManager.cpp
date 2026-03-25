#include "SessionManager.h"
#include "ClientSession.h"
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

void SessionManager::removeSession(int fd)
{
    std::lock_guard<std::mutex> lock(sessionMutex);

    // 1. 먼저 fd로 세션을 찾습니다.
    auto it = sessionMap.find(fd);
    if (it != sessionMap.end())
    {
        std::shared_ptr<ClientSession> session = it->second;

        // 🚀 [중요] 이 세션에 로그인된 userId가 있다면 userMap에서도 지워줍니다.
        // ClientSession 클래스에 getUserId() 함수가 있다고 가정합니다.
        std::string userId = session->getUserId();
        if (!userId.empty())
        {
            userMap.erase(userId);
            std::cout << "[SessionManager] 유저 '" << userId << "'의 논리적 연결 해제 (fd: " << fd << ")" << std::endl;
        }

        // 2. 물리적 세션 맵에서 삭제
        sessionMap.erase(it);
        std::cout << "[SessionManager] 소켓 " << fd << " 물리적 세션 제거 완료." << std::endl;
    }
}
std::shared_ptr<ClientSession> SessionManager::getSessionByUserId(const std::string &userId)
{
    std::lock_guard<std::mutex> lock(sessionMutex); // 🚀 안전제일! 자물쇠 잠그기

    auto it = userMap.find(userId);
    if (it != userMap.end())
    {
        return it->second; // 🚀 사장님이 접속 중이면 세션 리턴!
    }

    return nullptr; // 🚀 사장님이 앱을 끄고 오프라인 상태라면 nullptr 리턴
}

// 🚀 특정 유저ID로 패킷을 전송하고 성공 여부를 반환함
// [SessionManager.h] 내부 수정
