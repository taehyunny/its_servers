#include "PacketFramer.h"
#include "Global_protocol.h"
#include "Dispatcher.h"
#include <cstring>
#include <arpa/inet.h> // 🚀 ntohs, ntohl 함수 사용을 위해 필수!

void PacketFramer::onReceiveData(ClientSession *session, const char *data, int len, ThreadPool &pool)
{
    session->appendToReadBuffer(data, len);
    processBuffer(session, pool);
}

void PacketFramer::processBuffer(ClientSession *session, ThreadPool &pool)
{
    const auto &buffer = session->getReadBuffer();

    while (buffer.size() >= sizeof(PacketHeader))
    {
        PacketHeader header;
        std::memcpy(&header, buffer.data(), sizeof(PacketHeader));

        // 🚀 [필수] 네트워크 바이트(Big-Endian)를 호스트 바이트(Little-Endian)로 복구!
        header.signature = ntohs(header.signature);                                    // 시그니처는 검증용이므로 변환 후 비교
        header.cmdId = static_cast<CmdID>(ntohs(static_cast<uint16_t>(header.cmdId))); // CmdID는 enum class이므로 uint16_t로 캐스팅 후 ntohs 적용
        header.bodySize = ntohl(header.bodySize);                                      // bodySize는 4바이트이므로 ntohl로 변환

        if (header.signature != 0x4543)
        {
            // 비정상적인 접근은 연결 종료 처리 (로그만 남기고 탈출)
            return;
        }

        uint32_t totalPacketSize = sizeof(PacketHeader) + header.bodySize;
        if (buffer.size() < totalPacketSize)
        {
            break;
        }

        // [안전성 강화] iterator 대신 포인터 연산(data)을 사용해 메모리 연속성 보장
        std::string jsonBody(buffer.data() + sizeof(PacketHeader), header.bodySize);

        // 🚀 [수정] 디스패처에게 pool을 함께 전달
        Dispatcher::dispatch(session, header, jsonBody, pool);

        session->removeReadData(totalPacketSize);
    }
}