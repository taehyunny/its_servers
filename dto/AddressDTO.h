#pragma once
#include <string>
#include <vector>
#include "json.hpp"

// [2070] 주소 저장 요청
struct ReqAddressSaveDTO {
    std::string userId;
    std::string address;   // 도로명 주소
    std::string detail;    // 상세주소
    std::string guide;     // 길안내
    std::string label;     // "집" / "회사" / "기타"
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ReqAddressSaveDTO,
        userId, address, detail, guide, label)
};

// [2071] 주소 저장 응답
struct ResAddressSaveDTO {
    int status;
    int addressId;         // 저장된 주소 PK
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ResAddressSaveDTO, status, addressId)
};

// [2072] 주소 목록 조회 요청
struct ReqAddressListDTO {
    std::string userId;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ReqAddressListDTO, userId)
};

// 주소 목록 아이템 (응답에서 사용)
struct AddressItem {
    int         addressId;
    std::string address;
    std::string detail;
    std::string guide;
    std::string label;
    bool        isDefault;  // true: 현재 선택된 주소
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(AddressItem,
        addressId, address, detail, guide, label, isDefault)
};

// [2073] 주소 목록 조회 응답
struct ResAddressListDTO {
    int status;
    std::vector<AddressItem> addresses;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ResAddressListDTO, status, addresses)
};

// [2074] 주소 삭제 요청
struct ReqAddressDeleteDTO {
    std::string userId;
    int addressId;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ReqAddressDeleteDTO, userId, addressId)
};

// [2075] 주소 삭제 응답
struct ResAddressDeleteDTO {
    int status;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ResAddressDeleteDTO, status)
};

// [2076] 주소 수정 요청
struct ReqAddressUpdateDTO {
    std::string userId;
    int         addressId;
    std::string detail;
    std::string guide;
    std::string label;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ReqAddressUpdateDTO,
        userId, addressId, detail, guide, label)
};

// [2077] 주소 수정 응답
struct ResAddressUpdateDTO {
    int status;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ResAddressUpdateDTO, status)
};

// [2078] 기본 주소 설정 요청 (선택된 주소 변경)
struct ReqAddressDefaultDTO {
    std::string userId;
    int         addressId;  // 기본으로 설정할 주소 PK
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ReqAddressDefaultDTO, userId, addressId)
};

// [2079] 기본 주소 설정 응답
struct ResAddressDefaultDTO {
    int status;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ResAddressDefaultDTO, status)
};
