#pragma once
#include <memory>
#include <string>

// 전방 선언 (순환 참조 방지)
class ClientSession;

class SystemHandler
{
public:
    // 🚀 [CmdID: 1000] 하트비트 수신 및 메인 화면(카테고리/매출 1등 매장) 동기화
    static void handleHeartbeat(std::shared_ptr<ClientSession> session, const std::string &jsonBody);

    // 필요하다면 나중에 프로그램 종료나 기타 시스템 관련 핸들러를 여기에 추가할 수 있습니다.
};