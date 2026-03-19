#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include <mariadb/conncpp.hpp>

class AuthDAO
{
public:
    // DSDBManager와 동일한 싱글톤 구조 적용
    static AuthDAO &getInstance()
    {
        static AuthDAO instance;
        return instance;
    }

    // 로그인 검증 (ID/PW 확인 후 유저 정보 반환)
    nlohmann::json validateLogin(const std::string &userId, const std::string &password);

private:
    AuthDAO() = default;
    ~AuthDAO() = default;
};