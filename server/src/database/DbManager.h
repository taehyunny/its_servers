#pragma once
#include <mariadb/conncpp.hpp>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <string>

class DBManager
{
public:
    static DBManager &getInstance()
    {
        static DBManager instance;
        return instance;
    }

    // 서버 시작 시 DB 연결을 미리 생성해 두는 초기화 함수
    void init(const std::string &url, const std::string &user, const std::string &password, int poolSize = 5);

    // 🚀 스마트 포인터(RAII)를 활용해 다 쓰면 자동으로 풀에 반납되는 커넥션 대여 함수
    std::shared_ptr<sql::Connection> getConnection();

private:
    DBManager() = default;
    ~DBManager();

    // 복사 방지 (싱글톤 원칙)
    DBManager(const DBManager &) = delete;
    DBManager &operator=(const DBManager &) = delete;

    std::queue<std::unique_ptr<sql::Connection>> connectionPool;
    std::mutex poolMutex;
    std::condition_variable poolCondVar;

    std::string dbUrl;
    std::string dbUser;
    std::string dbPassword;
};