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
    std::string userId;   // 아이디
    std::string password; // 비밀번호
    std::string userName; // 유저 이름
    std::string phoneNumber; // 전화번호
    int role; // 0: 고객, 1: 사장님, 2: 라이더

    // 👤 [고객 전용 - Role 0]
    std::string address;  // 주소 (고객일 때만 채워짐)

    // 👨‍🍳 [사장님 & 매장 전용 - Role 1]
    std::string businessNumber;  // 사업자 등록번호 (사장님일 때만 채워짐)
    std::string accountNumber;  // 계좌 번호 (사장님일 때만 채워짐)
    std::string storeName;  // 매장 이름 (사장님일 때만 채워짐)
    std::string category;  // 매장 카테고리 (사장님일 때만 채워짐)
    std::string storeAddress;  // 매장 주소 (사장님일 때만 채워짐)

    friend void to_json(nlohmann::json &j, const SignupReqDTO &dto) // JSON <-> Struct 자동 변환 매크로 대신 커스텀 구현 (선택적 필드 처리)
    {
        j = nlohmann::json{ // [공통 정보]
            {"userId", dto.userId},   // 아이디
            {"password", dto.password},  // 비밀번호
            {"userName", dto.userName},  // 유저 이름
            {"phoneNumber", dto.phoneNumber},  // 전화번호
            {"role", dto.role},  // 역할

             // 👤 [고객 전용 - Role 0]
             {"address", dto.address},  // 주소 (고객일 때만 채워짐)

             // 👨‍🍳 [사장님 & 매장 전용 - Role 1]
            {"businessNumber", dto.businessNumber},  // 사업자 등록번호 (사장님일 때만 채워짐)
            {"accountNumber", dto.accountNumber},  // 계좌 번호 (사장님일 때만 채워짐)
            {"storeName", dto.storeName},  // 매장 이름 (사장님일 때만 채워짐)
            {"category", dto.category},  // 매장 카테고리 (사장님일 때만 채워짐)
            {"storeAddress", dto.storeAddress}};
    }

    // ⚠️ 매크로 대신 커스텀 파서 사용! (선택적 필드 처리)
    friend void from_json(const nlohmann::json &j, SignupReqDTO &dto) // JSON <-> Struct 자동 변환 매크로 대신 커스텀 구현 (선택적 필드 처리)
    {
        // [필수 값] 이건 없으면 에러 던짐 (at 사용)
        j.at("userId").get_to(dto.userId);  // 아이디
        j.at("password").get_to(dto.password);  // 비밀번호
        j.at("userName").get_to(dto.userName);  // 유저 이름
        j.at("phoneNumber").get_to(dto.phoneNumber); // 전화번호 
        j.at("role").get_to(dto.role); // 역할

         // [선택 값] 없으면 빈 문자열("")로 안전하게 채움 (value 사용)
        // 🚀 [선택 값] 없으면 빈 문자열("")로 안전하게 채움 (value 사용)
        dto.address = j.value("address", ""); // 주소 (고객일 때만 채워짐)
        dto.businessNumber = j.value("businessNumber", ""); // 사업자 등록번호 (사장님일 때만 채워짐)
        dto.accountNumber = j.value("accountNumber", "");  // 계좌 번호 (사장님일 때만 채워짐)
        dto.storeName = j.value("storeName", "");  // 매장 이름 (사장님일 때만 채워짐)
        dto.category = j.value("category", "");  // 매장 카테고리 (사장님일 때만 채워짐)
        dto.storeAddress = j.value("storeAddress", "");  // 매장 주소 (사장님일 때만 채워짐)
    }
};
// ---------------------------------------------------------
// [2] 로그인 요청 DTO (클라이언트 -> 서버)
// ---------------------------------------------------------
struct LoginReqDTO
{
    std::string userId;   // 아이디 (핸들러의 req.userId에 대응)
    std::string password; // 비밀번호 (핸들러의 req.password에 대응)
    int role;
    // JSON <-> Struct 자동 변환 매크로
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(LoginReqDTO, userId, password, role)
};

struct LoginResDTO
{
    int status;   // 200: 성공, 400: 클라이언트 오류, 500: 서버 오류 등
    std::string message; // 유저에게 보여줄 메시지 (예: "로그인 성공", "비밀번호가 틀렸습니다.")
    std::string userName;  // 유저 이름 (로그인 시 클라이언트에서 저장할 수 있도록 반환)
    std::string address;  // 주소 (로그인 시 클라이언트에서 저장할 수 있도록 반환)
    std::string phoneNumber;  // 전화번호 (로그인 시 클라이언트에서 저장할 수 있도록 반환)
    std::string role;  // 역할 (로그인 시 클라이언트에서 저장할 수 있도록 반환, "0": 고객, "1": 사장님, "2": 라이더)
    std::string storeName;  // 매장 이름 (로그인 시 클라이언트에서 저장할 수 있도록 반환)

    // 🚀 매크로 필수!
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(LoginResDTO, status, message, userName, address, phoneNumber, role, storeName)
};
// ------------------------------------------------------
// [3] 통합 인증 응답 DTO (서버 -> 클라이언트)
// ---------------------------------------------------------
// 회원가입과 로그인 모두 상태값(status), 메시지(message), 유저이름(userName)을 반환하므로 하나로 통합합니다.
struct AuthResDTO
{
    int status; // 200: 성공, 400: 클라이언트 오류, 500: 서버 오류 등
    std::string userId;  // 아이디 (로그인 시 클라이언트에서 저장할 수 있도록 반환)
    std::string address;  // 주소 (로그인 시 클라이언트에서 저장할 수 있도록 반환)
    std::string message; // 유저에게 보여줄 메시지 (예: "로그인 성공", "비밀번호가 틀렸습니다.")
    std::string userName;  // 유저 이름 (로그인 시 클라이언트에서 저장할 수 있도록 반환)
    std::string phoneNumber;  // 전화번호 (로그인 시 클라이언트에서 저장할 수 있도록 반환)
    std::string role;  // 역할 (로그인 시 클라이언트에서 저장할 수 있도록 반환, "0": 고객, "1": 사장님, "2": 라이더)
    std::string errorType;  // 400 에러일 때 어떤 필드가 문제인지 (예: "userId", "password", "phoneNumber" 등)

    // ── 사장님 전용 (role == "1" 일 때만 채워짐) ──────────────
    int storeId = 0; // 사업자번호 겸 매장 ID
    std::string businessNumber;  // 사업자 번호
    std::string storeName;  // 매장 이름
    std::string category;   // 매장 카테고리
    std::string storeAddress;  // 매장 주소
    std::string cookTime;    // 평균 조리 시간
    std::string minOrderAmount;  // 최소 주문 금액
    std::string openTime;   // 영업 시작 시간
    std::string closeTime;  // 영업 종료 시간
    std::string accountNumber;  // 계좌 번호
    int approvalStatus = 0; // 0: 대기, 1: 승인

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(AuthResDTO,
                                   status, message, userId, address, userName, phoneNumber, role, errorType,
                                   storeId, storeName, category, storeAddress,
                                   cookTime, minOrderAmount, openTime, closeTime,
                                   accountNumber, approvalStatus, businessNumber)
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

