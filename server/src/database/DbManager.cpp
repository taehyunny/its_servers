#include "DbManager.h"
#include <iostream>

void DBManager::init(const std::string &url, const std::string &user, const std::string &password, int poolSize)
{
    dbUrl = url;
    dbUser = user;
    dbPassword = password;

    try
    {
        // MariaDB 드라이버 획득
        sql::Driver *driver = sql::mariadb::get_driver_instance();

        // 지정된 개수(poolSize)만큼 커넥션을 미리 생성해서 큐(Queue)에 적재
        for (int i = 0; i < poolSize; ++i)
        {
            sql::SQLString urlStr(dbUrl.c_str());
            sql::Properties props({{"user", dbUser.c_str()},
                                   {"password", dbPassword.c_str()}});

            std::unique_ptr<sql::Connection> conn(driver->connect(urlStr, props));
            connectionPool.push(std::move(conn));
        }
        std::cout << "[DBManager] MariaDB 커넥션 풀 초기화 완료 (크기: " << poolSize << ")" << std::endl;
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "[FATAL] DB 커넥션 풀 초기화 실패: " << e.what() << std::endl;
        exit(EXIT_FAILURE); // DB가 죽으면 서버도 켜지지 않게 강제 종료
    }
}

std::shared_ptr<sql::Connection> DBManager::getConnection()
{
    std::unique_lock<std::mutex> lock(poolMutex);

    // 1. 남은 커넥션이 없으면, 다른 스레드가 반납할 때까지 대기
    poolCondVar.wait(lock, [this]
                     { return !connectionPool.empty(); });

    // 2. 큐에서 커넥션 하나 꺼내기
    std::unique_ptr<sql::Connection> conn = std::move(connectionPool.front());
    connectionPool.pop();

    // 3. 🚀 마법의 Custom Deleter: DAO에서 사용이 끝나면 큐로 자동 복귀
    return std::shared_ptr<sql::Connection>(conn.release(), [this](sql::Connection *c)
                                            {
                                                std::lock_guard<std::mutex> retLock(poolMutex);
                                                connectionPool.push(std::unique_ptr<sql::Connection>(c));
                                                poolCondVar.notify_one(); // 기다리고 있는 다른 스레드를 깨움
                                            });
}

DBManager::~DBManager()
{
    std::lock_guard<std::mutex> lock(poolMutex);
    while (!connectionPool.empty())
    {
        connectionPool.front()->close();
        connectionPool.pop();
    }
}