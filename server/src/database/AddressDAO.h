#pragma once
#include <string>
#include <vector>
#include <memory>
#include "AddressDTO.h" // 지나님이 주신 DTO 헤더
#include "DbManager.h"  // 대소문자 주의 (DBManager.h가 맞는지 확인)

class AddressDAO
{
private:
    AddressDAO() = default;
    ~AddressDAO() = default;

public:
    static AddressDAO &getInstance()
    {
        static AddressDAO instance;
        return instance;
    }

    // 1. 주소 저장 (성공 시 새로 발급된 address_id 반환, 실패 시 0)
    int saveAddress(const ReqAddressSaveDTO &req);

    // 2. 주소 목록 조회
    std::vector<AddressItemDTO> getAddressList(const std::string &userId);

    // 3. 주소 삭제 (기본 주소는 삭제 불가)
    bool deleteAddress(const std::string &userId, int addressId);

    // 4. 기본 주소 변경
    bool setDefaultAddress(const std::string &userId, int addressId);

    // 5. 주소 수정
    bool updateAddress(const ReqAddressUpdateDTO &req);

    // 복사 방지
    AddressDAO(const AddressDAO &) = delete;
    AddressDAO &operator=(const AddressDAO &) = delete;
};