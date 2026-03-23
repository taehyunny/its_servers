#pragma once
#include <string>
#include <vector>
#include <memory>
#include "AddressDTO.h" // 지나님이 주신 DTO 헤더
#include "DbManager.h"

class AddressDAO {
private:
    AddressDAO() = default;
public:
    static AddressDAO& getInstance() {
        static AddressDAO instance;
        return instance;
    }

    // 1. 주소 저장 (성공 시 새로 발급된 address_id 반환, 실패 시 0)
    int saveAddress(const ReqAddressSaveDTO& req);

    // 2. 주소 목록 조회
    std::vector<AddressItem> getAddressList(const std::string& userId);
};
