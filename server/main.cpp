// 1. 표준 라이브러리 (가장 기초가 되는 도구들)
#include <iostream>
#include <exception>
#include <csignal>
#include <thread>
#include <vector> // DTO나 DAO에서 쓸 수 있으니 확인

// 2. 프로젝트 공통 인프라 (서버의 뼈대)
#include "ItsServer.h"
#include "ThreadPool.h"
#include "ClientSession.h"
#include "DbManager.h" // DB 커넥션 풀 매니저
// 3. 데이터 모델 (그릇)
// DTO는 DAO나 Handler에서 참조하므로 위쪽에 두는 것이 안전합니다.
#include "AllDTOs.h"

// 4. 데이터 접근 객체 (국자)
#include "UserDAO.h"
#include "AuthDAO.h"
#include "StoreDAO.h"
#include "MenuDAO.h"

// 5. 비즈니스 로직 (요리사)
#include "UserHandler.h"
#include "StoreHandler.h"
#include "MenuHandler.h"
#include "OrderHandler.h"
#include "Dispatcher.h" // 요청을 각 Handler로 분배하는 역할

// =========================================================================
// [추가] 전역 종료 플래그
// Ctrl+C 또는 kill 명령이 들어오면 signalHandler 가 이 값을 false 로 바꿔
// server.run() 내부 루프가 자연스럽게 빠져나올 수 있게 합니다.
// volatile : 컴파일러 최적화로 인해 값이 무시되지 않도록 보장
// =========================================================================
volatile bool g_running = true; //  서버가 실행 중인지 여부를 나타내는 전역 변수 (true: 실행 중, false: 종료 신호 수신)

// [추가] 시그널 핸들러 함수
// SIGINT  (Ctrl+C) 또는 SIGTERM (kill 명령) 수신 시 호출됩니다.
// 서버가 강제 종료되지 않고 ThreadPool, DB 연결 등을 정리하고 끝낼 수 있습니다.
void signalHandler(int sig)
{
    std::cout << "\n[INFO] 서버 종료 신호 수신 (signal: " << sig << "). 정리 중..." << std::endl; // 시그널 번호 출력 (예: SIGINT=2, SIGTERM=15)
    g_running = false;                                                                            // 종료 플래그를 false로 설정하여 server.run() 내부 루프가 종료되도록 유도
}

// =========================================================================
// main
// =========================================================================
int main(int argc, char *argv[])
{
    // [추가] 시그널 핸들러 등록
    // signal() 을 main 맨 위에서 호출해야 서버 실행 전부터 종료 신호를 받을 수 있습니다.
    signal(SIGINT, signalHandler);  // Ctrl+C 처리
    signal(SIGTERM, signalHandler); // kill 명령 처리

    // ── 포트 파싱 ─────────────────────────────────────────────────────────
    int port = 8000;
    if (argc > 1)
    {
        try
        {
            port = std::stoi(argv[1]);

            // [추가] 포트 범위 검증
            // 0 ~ 1023 : 시스템 예약 포트 (root 권한 필요)
            // 65536 이상 : 유효하지 않은 포트 번호
            // 위 범위를 벗어나면 기본값(8080)으로 되돌립니다.
            if (port < 1024 || port > 65535)
            {
                std::cerr << "[WARNING] 포트 범위 오류 (" << port
                          << "). 기본값(8080)을 사용합니다." << std::endl;
                port = 8000;
            }
        }
        catch (const std::exception &)
        {
            std::cerr << "[WARNING] 잘못된 포트 번호. 기본값(8080)을 사용합니다." << std::endl;
        }
    }

    // [추가] CPU 코어 수 기반 스레드 수 자동 감지
    // hardware_concurrency() 는 현재 CPU 의 논리 코어 수를 반환합니다.
    // 환경에 따라 0을 반환할 수 있으므로 기본값(4)으로 보정합니다.
    // 기존의 하드코딩(4) 대신 실행 환경에 맞는 최적 스레드 수를 사용합니다.
    int threadCount = static_cast<int>(std::thread::hardware_concurrency()); // CPU 코어 수 감지
    if (threadCount <= 0)
        threadCount = 4; // 감지 실패 시 기본값 4로 설정

    // [수정] 부팅 메시지에 포트 / 스레드 수 정보 추가
    // 서버 시작 시 어떤 설정으로 동작하는지 한눈에 확인할 수 있습니다.
    std::cout << "===========================================" << std::endl;
    std::cout << "       [ Its Bedalyo Server Booting ]      " << std::endl;
    std::cout << "===========================================" << std::endl;
    std::cout << "[INFO] Port        : " << port << std::endl;
    std::cout << "[INFO] ThreadPool  : " << threadCount << " workers" << std::endl;
    std::cout << "===========================================" << std::endl;

    try
    { // 1. DB 커넥션 풀 초기화
        DBManager::getInstance().init("jdbc:mariadb://localhost:3306/its_bedalyo", "bedalyo", "1234", threadCount);

        // 2. [오늘의 성과] 데이터 로드 테스트 (서버 부팅 시 콘솔 출력용)
        std::cout << "-------------------------------------------" << std::endl;
        auto stores = StoreDAO::getInstance().getAllStores();
        // 황귱쟁반(ID: 1)의 메뉴가 잘 들어있는지도 한 번 찔러봅니다.
        auto menus = MenuDAO::getInstance().getMenusByStoreId(1);
        std::cout << "-------------------------------------------" << std::endl;

        // 3. 서버 실행 준비
        ThreadPool threadPool(threadCount);
        ItsServer server(port, threadPool);

        std::cout << "[INFO] 모든 핸들러 및 데이터 로드 완료. 서비스를 시작합니다." << std::endl;
        server.run(); // 서버의 메인 심장 (이벤트 루프)
    }
    catch (const std::exception &e)
    {
        std::cerr << "[FATAL] 서버 실행 중 예외 발생: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    // [추가] 정상 종료 메시지
    // 시그널을 받아 run() 이 정상적으로 종료되면 이 줄이 출력됩니다.
    std::cout << "[INFO] 서버가 정상 종료되었습니다." << std::endl;
    return EXIT_SUCCESS;
}