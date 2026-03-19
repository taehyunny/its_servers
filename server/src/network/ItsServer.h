#pragma once

#include <sys/epoll.h>

// 전방 선언 (헤더 파일의 무거움을 줄여줍니다)
class ThreadPool;
class SessionManager;

class ItsServer
{
private:
    static const int MAX_EVENTS = 10000; // epoll이 한 번에 처리할 최대 이벤트 수

    int serverFd;               // 대표 전화번호 (서버 소켓 FD)
    int epollFd;                // 문지기 (epoll FD)
    struct epoll_event *events; // 이벤트 바구니

    SessionManager *sessionManager; // 프론트 데스크 (세션 관리자)
    ThreadPool &threadPool;         // 워커 스레드 풀 (main에서 주입받음)

    // 내부 유틸리티 및 처리 함수
    void setNonBlocking(int sockFd);
    void acceptNewClient();
    void readFromClient(int clientFd);
    void disconnectClient(int clientFd);

public:
    // 생성자: 포트 번호와 ThreadPool을 외부(main)에서 주입받습니다.
    ItsServer(int port, ThreadPool &threadPool);

    // 소멸자: 서버 자원을 안전하게 해제합니다.
    ~ItsServer();

    // 서버의 메인 심장 (이벤트 루프)
    void run();

    // ❌ sendPacket 함수는 ClientSession 으로 이사 갔기 때문에 이곳에서 삭제되었습니다.
};