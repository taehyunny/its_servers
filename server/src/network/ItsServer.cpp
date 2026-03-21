#include "ItsServer.h"
#include "SessionManager.h"
#include "ClientSession.h"
#include "PacketFramer.h"
#include "CategoryHandler.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <memory> // 🚀 std::shared_ptr 사용을 위해 든든하게 추가!

// ==========================================================
// 1. 서버 초기화 (생성자)
// ==========================================================
ItsServer::ItsServer(int port, ThreadPool &threadPool) : threadPool(threadPool)
{
    sessionManager = &SessionManager::getInstance();
    events = new epoll_event[MAX_EVENTS];

    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd == -1)
    {
        std::cerr << "[FATAL] 서버 소켓 생성 실패" << std::endl;
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(port);

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

    setNonBlocking(serverFd);

    epollFd = epoll_create1(0);
    if (epollFd == -1)
    {
        std::cerr << "[FATAL] epoll_create1 실패" << std::endl;
        exit(EXIT_FAILURE);
    }

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
                continue;
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
void ItsServer::acceptNewClient()
{
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    while (true)
    {
        int clientFd = accept(serverFd, (struct sockaddr *)&clientAddr, &clientLen);

        if (clientFd == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break;
            }
            else
            {
                std::cerr << "[ERROR] accept 실패" << std::endl;
                break;
            }
        }

        setNonBlocking(clientFd);

        // 1. 세션 생성 (빈 테이블에 손님 앉히기)
        sessionManager->createSession(clientFd);

        // 🚀 2. 방금 만든 세션 객체(shared_ptr) 가져오기!
        // (태현님의 SessionManager에 getSession 같은 함수가 있다고 가정합니다. 이름이 다르면 맞춰주세요!)
        std::shared_ptr<ClientSession> newSession = sessionManager->getSession(clientFd);

        struct epoll_event event;
        event.events = EPOLLIN | EPOLLET;
        event.data.fd = clientFd;
        epoll_ctl(epollFd, EPOLL_CTL_ADD, clientFd, &event);

        char ipStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddr.sin_addr), ipStr, INET_ADDRSTRLEN);
        std::cout << "[INFO] 새 클라이언트 접속 완료! (fd: " << clientFd << ", IP: " << ipStr << ")" << std::endl;

        // -----------------------------------------------------------------
        // 🚀 3. 접속 성공하자마자 메인 데이터(1061번 패킷) 강제 발사!
        // -----------------------------------------------------------------
        if (newSession)
        {
            // (나중에는 Dispatcher처럼 ThreadPool에 넣어서 비동기로 쏘는 것이 베스트입니다!)
            try
            {
                // 클라이언트가 보낸 데이터가 없으므로 jsonBody는 빈 문자열("")을 줍니다.
                CategoryHandler::handleCategoryRequest(newSession, "");
                std::cout << "       -> 🎁 [서버 웰컴 선물] fd " << clientFd << " 에게 메인 화면 데이터 전송 완료!" << std::endl;
            }
            catch (const std::exception &e)
            {
                std::cerr << "       -> 🚨 메인 데이터 강제 전송 실패: " << e.what() << std::endl;
            }
        }
    }
}

// ==========================================================
// 6. 데이터 수신 및 분리 (Read & Frame)
// ==========================================================
void ItsServer::readFromClient(int clientFd)
{
    char buffer[4096];

    while (true)
    {
        int readBytes = read(clientFd, buffer, sizeof(buffer));

        if (readBytes > 0)
        {
            // 🚀 완벽하게 적용된 스마트 포인터!
            std::shared_ptr<ClientSession> session = sessionManager->getSession(clientFd);

            if (session != nullptr)
            {
                // 🚀 안전하게 Dispatcher와 ThreadPool까지 이어지는 데이터 흐름!
                PacketFramer::onReceiveData(session, buffer, readBytes, threadPool);
            }
        }
        else if (readBytes == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break;
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