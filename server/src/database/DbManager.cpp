#include "DbManager.h"
#include <iostream>

void DBManager::init(const std::string &url, const std::string &user, const std::string &password, int poolSize)
{
    dbUrl = url;
    dbUser = user;
    dbPassword = password;

    try
    {
        sql::Driver *driver = sql::mariadb::get_driver_instance();

        for (int i = 0; i < poolSize; ++i)
        {
            sql::SQLString urlStr(dbUrl.c_str());
            // 🚀 핵심 1: 커넥션 생성 시 자동 재연결 옵션(OPT_RECONNECT) 활성화
            sql::Properties props({{"user", dbUser.c_str()},
                                   {"password", dbPassword.c_str()},
                                   {"OPT_RECONNECT", "true"}});

            std::unique_ptr<sql::Connection> conn(driver->connect(urlStr, props));
            connectionPool.push(std::move(conn));
        }
        std::cout << "[DBManager] MariaDB 커넥션 풀 초기화 완료 (크기: " << poolSize << ")" << std::endl;
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "[FATAL] DB 커넥션 풀 초기화 실패: " << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }
}

std::shared_ptr<sql::Connection> DBManager::getConnection()
{
    std::unique_lock<std::mutex> lock(poolMutex);

    poolCondVar.wait(lock, [this]
                     { return !connectionPool.empty(); });

    std::unique_ptr<sql::Connection> conn = std::move(connectionPool.front());
    connectionPool.pop();

    // 🚀 핵심 2: 꺼낸 커넥션이 유효한지 검사하고, 죽어있다면 재연결!
    try
    {
        if (conn == nullptr || !conn->isValid())
        {
            std::cout << "[DBManager] ⚠️ 끊어진 커넥션 발견! 재연결을 시도합니다..." << std::endl;

            sql::Driver *driver = sql::mariadb::get_driver_instance();
            sql::SQLString urlStr(dbUrl.c_str());
            sql::Properties props({{"user", dbUser.c_str()},
                                   {"password", dbPassword.c_str()},
                                   {"OPT_RECONNECT", "true"}});

            conn.reset(driver->connect(urlStr, props));
            std::cout << "[DBManager] ✅ 재연결 성공!" << std::endl;
        }
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "[DBManager] ❌ 재연결 실패: " << e.what() << std::endl;
        // 실패 시 다시 큐에 넣지 않고 에러 처리를 하거나 새 커넥션을 기다리게 설계 가능
    }

    return std::shared_ptr<sql::Connection>(conn.release(), [this](sql::Connection *c)
                                            {
        std::lock_guard<std::mutex> retLock(poolMutex);
        connectionPool.push(std::unique_ptr<sql::Connection>(c));
        poolCondVar.notify_one(); });
}

DBManager::~DBManager()
{
    std::lock_guard<std::mutex> lock(poolMutex);
    while (!connectionPool.empty())
    {
        if (connectionPool.front())
        {
            connectionPool.front()->close();
        }
        connectionPool.pop();
    }
}