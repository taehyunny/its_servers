#pragma once
#include <unordered_map>

// ClientSession.h가 필요하다면 포함,
// 포인터만 사용한다면 전방 선언(class ClientSession;)으로 대체 가능합니다.
#include "ClientSession.h"

class SessionManager
{
public:
    void createSession(int fd); // 중괄호 { } 대신 세미콜론 ; 사용
    ClientSession *getSession(int fd);
    void removeSession(int fd);

private:
    std::unordered_map<int, ClientSession *> sessionMap;
};