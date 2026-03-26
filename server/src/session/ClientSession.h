#pragma once
#include <vector>
#include <string>
#include <mutex>
#include <unistd.h>
#include <arpa/inet.h> // htons, htonl을 위해 필수 포함
#include <nlohmann/json.hpp>
#include "Global_protocol.h"

class ClientSession : public std::enable_shared_from_this<ClientSession>
{
public:
    explicit ClientSession(int fd);
    ~ClientSession();

    // [역할 1, 2, 3] 버퍼 관리 (cpp 파일에서 구현)
    void appendToReadBuffer(const char *data, size_t len);
    const std::vector<char> &getReadBuffer() const;
    void removeReadData(size_t len);

    // [역할 4] 유저의 신원 보존 및 제공
    void authenticate(const std::string &id, int userRole);
    void setRole(int r) { role = r; } // 🚀 [수정] _role 오타 수정

    // 핸들러들이 유저 정보를 조회할 수 있는 Getter
    int getFd() const { return clientFd; }
    std::string getUserId() const { return userId; }
    int getRole() const { return role; }
    bool isAuthenticated() const { return isAuth; }

    // [역할 5] 템플릿 기반 만능 패킷 전송기
    template <typename T>
    void sendPacket(uint16_t cmdId, const T &dto)
    {
        try
        {
            nlohmann::json j = dto;
            std::string jsonBody = j.dump();

            PacketHeader header;
            // 필수 엔디안 변환 및 시그니처 삽입
            header.signature = htons(0x4543);                                       // E C (Eats Connect)
            header.cmdId = static_cast<CmdID>(htons(static_cast<uint16_t>(cmdId))); // 네트워크 바이트 순서로 변환
            header.bodySize = htonl(static_cast<uint32_t>(jsonBody.length()));      // JSON 바디 크기도 네트워크 바이트 순서로 변환

            std::vector<char> sendBuffer;
            sendBuffer.insert(sendBuffer.end(), reinterpret_cast<char *>(&header), reinterpret_cast<char *>(&header) + sizeof(PacketHeader));
            sendBuffer.insert(sendBuffer.end(), jsonBody.begin(), jsonBody.end());

            // Partial Write 방어를 포함한 안전한 전송 루프
            std::lock_guard<std::mutex> lock(writeMutex);

            size_t totalToSend = sendBuffer.size();
            size_t totalSent = 0;

            while (totalSent < totalToSend)
            {
                ssize_t sent = write(clientFd, sendBuffer.data() + totalSent, totalToSend - totalSent);
                if (sent > 0)
                {
                    totalSent += sent;
                }
                else if (sent == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
                {
                    // 논블로킹 소켓 버퍼가 가득 참. 임시 중단
                    break;
                }
                else
                {
                    // 소켓 에러 또는 클라이언트 종료
                    break;
                }
            }
        }
        catch (const std::exception &e)
        {
            // 직렬화 실패 등 예외 로깅
        }
    }

    void clearAuth()
    {
        std::lock_guard<std::mutex> lock(sessionMutex);
        isAuth = false;
        userId = "";
        role = 0;
    }

private:
    int clientFd;
    std::vector<char> readBuffer;

    bool isAuth;
    std::string userId;
    int role; // 🚀 [유지] 변수명 통일

    std::mutex sessionMutex;
    std::mutex writeMutex;
};