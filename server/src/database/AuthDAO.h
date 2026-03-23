#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include <mariadb/conncpp.hpp>
#include <utility>
#include "AllDTOs.h"
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
    std::pair<LoginResult, nlohmann::json> validateLogin(const std::string &userId, const std::string &password, int role); // 🚀 role 매개변수 추가

private:
    AuthDAO() = default;
    ~AuthDAO() = default;
};