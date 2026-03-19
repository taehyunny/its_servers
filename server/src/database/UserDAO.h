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

    // 회원가입: USERS 테이블에 새로운 레코드 삽입
    bool insertUser(const SignupReqDTO &dto);

private:
    UserDAO() = default;
    ~UserDAO() = default;
    UserDAO(const UserDAO &) = delete;
    UserDAO &operator=(const UserDAO &) = delete;
};