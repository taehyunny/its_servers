#include "PacketFramer.h"
#include "Global_protocol.h"
#include "Dispatcher.h"
#include <cstring>
#include <arpa/inet.h> // 🚀 ntohs, ntohl 함수 사용을 위해 필수!
#include <iostream>

void PacketFramer::onReceiveData(std::shared_ptr<ClientSession> session, const char *buffer, int length, ThreadPool &pool)
{
    session->appendToReadBuffer(buffer, length);

    // 이 클래스의 static 멤버 함수이므로 바로 호출 가능합니다.
    processBuffer(session, pool);
}

// 🚀 잃어버렸던 여는 중괄호 '{' 부활!
void PacketFramer::processBuffer(std::shared_ptr<ClientSession> session, ThreadPool &pool)
{
    const auto &buffer = session->getReadBuffer();

    while (buffer.size() >= sizeof(PacketHeader))
    {
        PacketHeader header;
        std::memcpy(&header, buffer.data(), sizeof(PacketHeader));

        // std::cout << "[PacketFramer] 현재 버퍼 크기: " << buffer.size() << " bytes" << std::endl;

        // 🚀 [필수] 네트워크 바이트(Big-Endian)를 호스트 바이트(Little-Endian)로 복구!
        header.signature = ntohs(header.signature);                                    // 시그니처는 검증용이므로 변환 후 비교
        header.cmdId = static_cast<CmdID>(ntohs(static_cast<uint16_t>(header.cmdId))); // CmdID는 enum class이므로 캐스팅 후 변환
        header.bodySize = ntohl(header.bodySize);                                      // bodySize는 4바이트이므로 ntohl로 변환

        if (header.signature != 0x4543)
        {
            std::cerr << "[PacketFramer] 🚨 비정상 패킷 감지! (잘못된 시그니처). 세션을 종료해야 합니다." << std::endl;
            // 실무에서는 여기서 버퍼를 다 비우거나, session을 강제 disconnect 시키는 로직을 호출해야 안전합니다.
            session->removeReadData(buffer.size()); // 쓰레기 데이터 싹 비우기
            return;
        }

        uint32_t totalPacketSize = sizeof(PacketHeader) + header.bodySize;
        if (buffer.size() < totalPacketSize)
        {
            break; // 아직 데이터가 다 안 왔으니 다음 기회에!
        }

        // [안전성 강화] iterator 대신 포인터 연산(data)을 사용해 메모리 연속성 보장
        std::string jsonBody(buffer.data() + sizeof(PacketHeader), header.bodySize);

        // 🚀 [수정] 디스패처에게 안전한 스마트 포인터(shared_ptr)와 pool을 함께 전달
        Dispatcher::dispatch(session, header, jsonBody, pool);

        session->removeReadData(totalPacketSize);
    }
} // 🚀 닫는 중괄호 위치 정상화!