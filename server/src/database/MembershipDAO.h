#pragma once
#include <string>
#include <vector>
#include <memory>
#include "AllDTOs.h" // DTO 정의가 포함된 헤더

class MembershipDAO
{
public:
    // 🚀 싱글톤 인스턴스 반환
    static MembershipDAO &getInstance()
    {
        static MembershipDAO instance;
        return instance;
    }

    // 🚀 등급 변경 및 테이블 동기화 (트랜잭션 처리)
    bool updateMembershipGrade(const std::string &userId, const std::string &targetGrade, int action);

private:
    MembershipDAO() = default;
    ~MembershipDAO() = default;
    MembershipDAO(const MembershipDAO &) = delete;
    MembershipDAO &operator=(const MembershipDAO &) = delete;
};