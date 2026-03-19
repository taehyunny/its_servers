#include "ClientSession.h"

ClientSession::ClientSession(int fd) : clientFd(fd), isAuth(false), role(0) {}

ClientSession::~ClientSession()
{
    // SessionManager가 close(clientFd)를 해주므로 여기선 비워두거나 안전망만 구축
}

void ClientSession::appendToReadBuffer(const char *data, size_t len)
{
    std::lock_guard<std::mutex> lock(sessionMutex); // 버퍼 동시 접근 보호
    readBuffer.insert(readBuffer.end(), data, data + len);
}

const std::vector<char> &ClientSession::getReadBuffer() const
{
    // 읽기 전용 참조 반환
    return readBuffer;
}

void ClientSession::removeReadData(size_t len)
{
    std::lock_guard<std::mutex> lock(sessionMutex);
    if (len <= readBuffer.size())
    {
        readBuffer.erase(readBuffer.begin(), readBuffer.begin() + len);
    }
}

void ClientSession::authenticate(const std::string &id, int userRole)
{
    std::lock_guard<std::mutex> lock(sessionMutex);
    userId = id;
    role = userRole;
    isAuth = true;
}