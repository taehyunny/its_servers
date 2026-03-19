#include "ItsServer.h"
#include "SessionManager.h"
#include "ClientSession.h"
#include "PacketFramer.h"

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>

// ==========================================================
// 1. 서버 초기화 (생성자)
// ==========================================================
ItsServer::ItsServer(int port, ThreadPool &threadPool) : threadPool(threadPool) // ThreadPool을 외부에서 주입받습니다.
{
    sessionManager = new SessionManager(); // 세션 관리자 초기화
    events = new epoll_event[MAX_EVENTS];  // 이벤트 바구니 할당

    // 1. 대표 전화 개통 (TCP 소켓)
    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd == -1)
    {
        std::cerr << "[FATAL] 서버 소켓 생성 실패" << std::endl;
        exit(EXIT_FAILURE);
    }

    // 2. 주소 할당 및 바인딩
    struct sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(port);

    // 포트 재사용 옵션 (서버 재시작 시 포트 충돌 방지)
    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(serverFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
    {
        std::cerr << "[FATAL] 포트 " << port << " 바인딩 실패 (이미 사용 중일 수 있음)" << std::endl;
        exit(EXIT_FAILURE);
    }

    if (listen(serverFd, SOMAXCONN) == -1)
    {
        std::cerr << "[FATAL] listen 실패" << std::endl;
        exit(EXIT_FAILURE);
    }

    // 서버 소켓을 논블로킹 모드로 전환
    setNonBlocking(serverFd);

    // epoll 문지기 고용
    epollFd = epoll_create1(0);
    if (epollFd == -1)
    {
        std::cerr << "[FATAL] epoll_create1 실패" << std::endl;
        exit(EXIT_FAILURE);
    }

    // 서버 소켓을 epoll 감시망에 등록 (엣지 트리거 방식)
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = serverFd;

    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, serverFd, &event) == -1)
    {
        std::cerr << "[FATAL] epoll_ctl (서버 소켓 등록) 실패" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::cout << "[ItsServer] 포트 " << port << "에서 이츠 배달료 서버 정상 가동!" << std::endl;
}

// ==========================================================
// 2. 서버 자원 해제 (소멸자)
// ==========================================================
ItsServer::~ItsServer()
{
    close(serverFd);
    close(epollFd);
    delete[] events;
    delete sessionManager;
    std::cout << "[ItsServer] 서버가 안전하게 종료되었습니다." << std::endl;
}

// ==========================================================
// 3. 논블로킹 설정 유틸리티
// ==========================================================
void ItsServer::setNonBlocking(int sockFd)
{
    int flags = fcntl(sockFd, F_GETFL, 0);
    if (flags == -1)
        return;
    fcntl(sockFd, F_SETFL, flags | O_NONBLOCK);
}

// ==========================================================
// 4. 메인 이벤트 루프 (생명 주기)
// ==========================================================
void ItsServer::run()
{
    while (true)
    {
        int event_count = epoll_wait(epollFd, events, MAX_EVENTS, -1);

        if (event_count == -1)
        {
            if (errno == EINTR)
                continue; // 시스템 인터럽트 발생 시 무시하고 계속 진행
            std::cerr << "[ERROR] epoll_wait 오류 발생" << std::endl;
            break;
        }

        for (int i = 0; i < event_count; ++i)
        {
            int currentFd = events[i].data.fd;

            if (currentFd == serverFd)
            {
                acceptNewClient();
            }
            else
            {
                readFromClient(currentFd);
            }
        }
    }
}

// ==========================================================
// 5. 신규 클라이언트 수락 (Accept)
// ==========================================================
void ItsServer::acceptNewClient() // 서버 소켓에 새로운 연결이 들어왔을 때 호출됩니다.
{
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    // 엣지 트리거이므로, 대기열에 있는 모든 손님을 한 번에 다 받음
    while (true)
    {
        int clientFd = accept(serverFd, (struct sockaddr *)&clientAddr, &clientLen);

        if (clientFd == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break; // 더 이상 수락할 손님이 없으면 탈출
            }
            else
            {
                std::cerr << "[ERROR] accept 실패" << std::endl;
                break;
            }
        }

        setNonBlocking(clientFd);

        // 프론트 데스크에 새 객실 등록 (Session 생성)
        sessionManager->createSession(clientFd);

        // 새 클라이언트 소켓을 epoll 감시망에 등록
        struct epoll_event event;
        event.events = EPOLLIN | EPOLLET;
        event.data.fd = clientFd;
        epoll_ctl(epollFd, EPOLL_CTL_ADD, clientFd, &event);

        char ipStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddr.sin_addr), ipStr, INET_ADDRSTRLEN);
        std::cout << "[INFO] 새 클라이언트 접속 완료! (fd: " << clientFd << ", IP: " << ipStr << ")" << std::endl;
    }
}

// ==========================================================
// 6. 데이터 수신 및 분리 (Read & Frame)
// ==========================================================
void ItsServer::readFromClient(int clientFd)
{
    char buffer[4096];

    // 논블로킹 소켓이므로, 버퍼에 있는 데이터를 남김없이 다 긁어와야 함
    while (true)
    {
        int readBytes = read(clientFd, buffer, sizeof(buffer));

        if (readBytes > 0)
        {
            ClientSession *session = sessionManager->getSession(clientFd); // 프론트 데스크에서 해당 FD의 세션을 찾아옵니다.
            if (session != nullptr)
            {
                // PacketFramer에게 조립 위임
                PacketFramer::onReceiveData(session, buffer, readBytes, threadPool); // 🚀 ThreadPool도 함께 전달
            }
        }
        else if (readBytes == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break; // 소켓 버퍼를 다 비웠음
            }
            else
            {
                std::cerr << "[ERROR] read 실패" << std::endl;
                disconnectClient(clientFd);
                break;
            }
        }
        else
        {
            // readBytes == 0 이면 클라이언트가 정상적으로 FIN을 보내 연결을 종료한 것
            disconnectClient(clientFd);
            break;
        }
    }
}

// ==========================================================
// 7. 클라이언트 접속 종료
// ==========================================================
void ItsServer::disconnectClient(int clientFd)
{
    std::cout << "[INFO] 클라이언트 접속 종료 (fd: " << clientFd << ")" << std::endl;
    sessionManager->removeSession(clientFd);
    epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, nullptr);
    close(clientFd);
}