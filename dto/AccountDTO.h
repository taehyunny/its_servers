#pragma once
#include <string>
// #include <nlohmann/json.hpp>
#include "json.hpp" // nlohmann json
// ---------------------------------------------------------
// [1] 회원가입 요청 DTO (클라이언트 -> 서버)
// ---------------------------------------------------------
struct SignupReqDTO
{
    // [공통 정보]
    std::string userId;
    std::string password;
    std::string userName;
    std::string phoneNumber;
    int role; // 0: 고객, 1: 사장님, 2: 라이더

    // 👤 [고객 전용 - Role 0]
    std::string address;

    // 👨‍🍳 [사장님 & 매장 전용 - Role 1]
    std::string businessNumber;
    std::string accountNumber;
    std::string storeName;
    std::string category;
    std::string storeAddress;

    friend void to_json(nlohmann::json &j, const SignupReqDTO &dto)
    {
        j = nlohmann::json{
            {"userId", dto.userId},
            {"password", dto.password},
            {"userName", dto.userName},
            {"phoneNumber", dto.phoneNumber},
            {"role", dto.role},
            {"address", dto.address},
            {"businessNumber", dto.businessNumber},
            {"accountNumber", dto.accountNumber},
            {"storeName", dto.storeName},
            {"category", dto.category},
            {"storeAddress", dto.storeAddress}};
    }

    // ⚠️ 매크로 대신 커스텀 파서 사용! (선택적 필드 처리)
    friend void from_json(const nlohmann::json &j, SignupReqDTO &dto)
    {
        // [필수 값] 이건 없으면 에러 던짐 (at 사용)
        j.at("userId").get_to(dto.userId);
        j.at("password").get_to(dto.password);
        j.at("userName").get_to(dto.userName);
        j.at("phoneNumber").get_to(dto.phoneNumber);
        j.at("role").get_to(dto.role);

        // 🚀 [선택 값] 없으면 빈 문자열("")로 안전하게 채움 (value 사용)
        dto.address = j.value("address", "");
        dto.businessNumber = j.value("businessNumber", "");
        dto.accountNumber = j.value("accountNumber", "");
        dto.storeName = j.value("storeName", "");
        dto.category = j.value("category", "");
        dto.storeAddress = j.value("storeAddress", "");
    }
};
// ---------------------------------------------------------
// [2] 로그인 요청 DTO (클라이언트 -> 서버)
// ---------------------------------------------------------
struct LoginReqDTO
{
    std::string userId;   // 아이디 (핸들러의 req.userId에 대응)
    std::string password; // 비밀번호 (핸들러의 req.password에 대응)

    // JSON <-> Struct 자동 변환 매크로
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(LoginReqDTO, userId, password)
};

struct LoginResDTO
{
    int status;
    std::string message;
    std::string userName;
    std::string address;
    std::string phoneNumber;
    std::string role;
    std::string storeName;

    // 🚀 매크로 필수!
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(LoginResDTO, status, message, userName, address, phoneNumber, role, storeName)
};
// ------------------------------------------------------
// [3] 통합 인증 응답 DTO (서버 -> 클라이언트)
// ---------------------------------------------------------
// 회원가입과 로그인 모두 상태값(status), 메시지(message), 유저이름(userName)을 반환하므로 하나로 통합합니다.
struct AuthResDTO
{
    int status;
    std::string userId;      // 200(성공), 400(잘못된 요청), 401(비번틀림), 404(아이디없음), 409(중복)
    std::string message;     // 유저에게 보여줄 알림창 문구
    std::string userName;    // 성공 시에만 채워줌
    std::string phoneNumber; // 로그인 성공 시 클라이언트에 폰번호도 같이 보내주면, 로그인 후 화면에서 환영 메시지에 활용 가능!
    std::string role;        // 로그인 성공 시 클라이언트에 역할도 같이 보내주면, 화면에서 "사장님 환영합니다!" / "고객님 환영합니다!" 등으로 활용 가능!
    std::string storeName;   // 사장님 로그인 시 매장 이름도 같이 보내주면, 화면에서 "OOO님 환영합니다!" 등으로 활용 가능! (사장님이 아닌 경우는 빈 문자열(""))
    std::string address;     // 고객 로그인 시 주소도 같이 보내주면, 화면에서 "OOO님 환영합니다! 배달 주소: XXX" 등으로 활용 가능! (사장님이 아닌 경우는 빈 문자열(""))
    std::string errorType;   // "DUPLICATE_ID", "WRONG_PASSWORD" 등 로직 처리용 키워드

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(AuthResDTO, status, userId, message, userName, address, phoneNumber, role, storeName, errorType)
};
// ---------------------------------------------------------
// [5] 중복 확인 DTO (1040 ~ 1043)
// ---------------------------------------------------------

// 1040: 아이디 중복 확인 요청
struct AuthCheckReqDTO
{
    std::string userId; // 검사할 아이디 (핸들러의 req.userId에 대응)
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(AuthCheckReqDTO, userId)
};

// 1041: 아이디 중복 확인 응답 (폰번호 응답과 구조가 같아서 같이 써도 되지만, 명확성을 위해 분리)
struct AuthCheckResDTO
{
    int status;          // 200(사용가능), 409(중복)
    bool isAvailable;    // 🚀 핵심: 사용 가능 여부 (true: 사용 가능, false: 중복)
    std::string message; //    유저에게 보여줄 메시지 (예: "이미 사용 중인 아이디입니다.")
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(AuthCheckResDTO, status, isAvailable, message)
};

// 1042: 폰번호 중복 확인 요청
struct PhoneCheckReqDTO
{
    std::string phoneNumber;                                            // 전화번호 (핸들러의 req.phoneNumber에 대응)
    int role;                                                           // 🚀 핵심: 사장님(1)인지 손님(0)인지 구분!
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(PhoneCheckReqDTO, phoneNumber, role) // JSON <-> Struct 자동 변환 매크로
};

// 1043: 폰번호 중복 확인 응답
struct PhoneCheckResDTO
{
    int status;                                                                    // 200(사용가능), 409(중복)
    bool isAvailable;                                                              // 🚀 핵심: 사용 가능 여부 (true: 사용 가능, false: 중복)
    std::string message;                                                           // 유저에게 보여줄 메시지 (예: "이미 사용 중인 전화번호입니다.")
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(PhoneCheckResDTO, status, isAvailable, message) // JSON <-> Struct 자동 변환 매크로
};