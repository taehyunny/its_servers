#pragma once
#include <mariadb/conncpp.hpp>
#include "AllDTOs.h"
#include <string>

class UserDAO
{
public:
    static UserDAO &getInstance()
    {
        static UserDAO instance;
        return instance;
    }
    bool existsById(const std::string &userId);
    bool existsByPhoneAndRole(const std::string &phoneNumber, int role); // 🚀 이름과 매개변수 변경
    // 회원가입: USERS 테이블에 새로운 레코드 삽입
    SignupResult insertUser(const SignupReqDTO &dto);
    bool existsByBizNum(const std::string &businessNum);
    std::pair<LoginResult, nlohmann::json> checkLogin(const std::string &userId, const std::string &password); // 로그인 검증 및 유저 정보 반환
    bool executeUpdate(const std::string &query, const std::vector<std::string> &params);
    std::string getUserGrade(const std::string &userId);

private:
    UserDAO() = default;
    ~UserDAO() = default;
    UserDAO(const UserDAO &) = delete;
    UserDAO &operator=(const UserDAO &) = delete;
};