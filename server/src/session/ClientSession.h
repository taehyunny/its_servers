#pragma once
#include <vector>
#include <string>
#include <mutex>
#include <unistd.h>
#include <arpa/inet.h> // htons, htonl을 위해 필수 포함
#include <nlohmann/json.hpp>
#include "Global_protocol.h"

class ClientSession
{
public:
    explicit ClientSession(int fd);
    ~ClientSession(); // 소멸자에서 소켓 닫기 등을 처리할 수 있음

    // [역할 1, 2, 3] 버퍼 관리 (cpp 파일에서 구현)
    void appendToReadBuffer(const char *data, size_t len);
    const std::vector<char> &getReadBuffer() const;
    void removeReadData(size_t len);

    // [역할 4] 유저의 신원 보존 및 제공
    void authenticate(const std::string &id, int userRole);

    // 🚀 [추가] 핸들러들이 유저 정보를 조회할 수 있는 Getter
    int getFd() const { return clientFd; }
    std::string getUserId() const { return userId; }
    int getRole() const { return role; }
    bool isAuthenticated() const { return isAuth; }

    // [역할 5] 템플릿 기반 만능 패킷 전송기 (반드시 헤더에 구현)
    template <typename T>
    void sendPacket(uint16_t cmdId, const T &dto) // CmdID를 uint16_t로 받음
    {
        try
        {
            nlohmann::json j = dto;
            std::string jsonBody = j.dump();

            PacketHeader header;
            // 🚀 [보완] 필수 엔디안 변환 및 시그니처 삽입
            header.signature = htons(0x4543);                                       // E C (Eats Connect)
            header.cmdId = static_cast<CmdID>(htons(static_cast<uint16_t>(cmdId))); // 명령어 ID도 네트워크 바이트 순서로 변환
            header.bodySize = htonl(static_cast<uint32_t>(jsonBody.length()));      // JSON 바디 크기도 네트워크 바이트 순서로 변환

            std::vector<char> sendBuffer; // 패킷 전체를 담을 버퍼
            sendBuffer.insert(sendBuffer.end(), reinterpret_cast<char *>(&header), reinterpret_cast<char *>(&header) + sizeof(PacketHeader));
            sendBuffer.insert(sendBuffer.end(), jsonBody.begin(), jsonBody.end());

            // 🚀 [보완] Partial Write 방어를 포함한 안전한 전송 루프
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
                    // 논블로킹 소켓 버퍼가 가득 참. 임시 중단 (상용에서는 별도 큐잉 필요)
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
    int role;

    std::mutex sessionMutex;
    std::mutex writeMutex;
};
