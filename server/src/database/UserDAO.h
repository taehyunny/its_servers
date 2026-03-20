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

private:
    UserDAO() = default;
    ~UserDAO() = default;
    UserDAO(const UserDAO &) = delete;
    UserDAO &operator=(const UserDAO &) = delete;
};