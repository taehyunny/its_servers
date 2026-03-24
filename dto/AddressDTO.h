#pragma once

#include <string>
#include <vector>
#include "json.hpp"

// 주소 목록 아이템 (조회 응답에서 사용)

struct AddressItem
{

    int addressId = 0;
    std::string address;
    std::string detail;
    std::string guide;
    std::string label = "기타";
    bool isDefault = false;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(AddressItem,

                                   addressId, address, detail, guide, label, isDefault)
};

// [2070] 주소 저장 요청

struct ReqAddressSaveDTO
{

    std::string userId;
    std::string address;
    std::string detail;
    std::string guide;
    std::string label;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ReqAddressSaveDTO,

                                   userId, address, detail, guide, label)
};

// [2071] 주소 저장 응답

struct ResAddressSaveDTO
{

    int status = 200;
    int addressId = 0;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ResAddressSaveDTO, status, addressId)
};

// [2072] 주소 목록 조회 요청

struct ReqAddressListDTO
{

    std::string userId;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ReqAddressListDTO, userId)
};

// [2073] 주소 목록 조회 응답

struct ResAddressListDTO
{

    int status = 200;
    std::vector<AddressItem> addresses;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ResAddressListDTO, status, addresses)
};

// [2074] 주소 삭제 요청

struct ReqAddressDeleteDTO
{

    std::string userId;
    int addressId = 0;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ReqAddressDeleteDTO, userId, addressId)
};

// [2075] 주소 삭제 응답

struct ResAddressDeleteDTO
{

    int status = 200;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ResAddressDeleteDTO, status)
};

// [2076] 주소 수정 요청 (상세주소, 길안내, 라벨 수정)

struct ReqAddressUpdateDTO
{

    std::string userId;
    int addressId = 0;
    std::string detail;
    std::string guide;
    std::string label;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ReqAddressUpdateDTO,

                                   userId, addressId, detail, guide, label)
};

// [2077] 주소 수정 응답

struct ResAddressUpdateDTO
{

    int status = 200;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ResAddressUpdateDTO, status)
};

// [2078] 기본 주소 변경 요청 (선택된 주소 변경)

struct ReqAddressDefaultDTO
{

    std::string userId;

    int addressId = 0;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ReqAddressDefaultDTO, userId, addressId)
};

// [2079] 기본 주소 변경 응답

struct ResAddressDefaultDTO
{

    int status = 200;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ResAddressDefaultDTO, status)
};