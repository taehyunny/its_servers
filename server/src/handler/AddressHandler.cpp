#include "AddressHandler.h"
#include "AddressDAO.h"
#include "ClientSession.h"
#include "Global_protocol.h"
#include <nlohmann/json.hpp>
#include <iostream>

using nlohmann::json;

void AddressHandler::handleAddressSave(std::shared_ptr<ClientSession> session, const std::string& jsonBody) {
    try {
        auto req = json::parse(jsonBody).get<ReqAddressSaveDTO>();
        int newAddressId = AddressDAO::getInstance().saveAddress(req);

        ResAddressSaveDTO res;
        if (newAddressId > 0) {
            res.status = 200;
            res.addressId = newAddressId;
        } else {
            res.status = 500;
            res.addressId = 0;
        }
        
        json resJson = res;
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ADDRESS_SAVE), resJson);
    } catch (const std::exception& e) {
        std::cerr << "[AddressHandler] Save Error: " << e.what() << std::endl;
    }
}

void AddressHandler::handleAddressList(std::shared_ptr<ClientSession> session, const std::string& jsonBody) {
    try {
        auto req = json::parse(jsonBody).get<ReqAddressListDTO>();
        auto addresses = AddressDAO::getInstance().getAddressList(req.userId);

        ResAddressListDTO res;
        res.status = 200;
        res.addresses = addresses;

        json resJson = res;
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ADDRESS_LIST), resJson);
    } catch (const std::exception& e) {
        std::cerr << "[AddressHandler] List Error: " << e.what() << std::endl;
    }
}

// ── 3. 주소 삭제 (공사 중 껍데기) ─────────────────────────
void AddressHandler::handleAddressDelete(std::shared_ptr<ClientSession> session, const std::string& jsonBody) {
    std::cout << "[AddressHandler] Delete API 호출됨 (현재 구현 중)" << std::endl;
    nlohmann::json res;
    res["status"] = 501; // 501: Not Implemented (기능 구현 안 됨)
    session->sendPacket(static_cast<uint16_t>(CmdID::RES_ADDRESS_DELETE), res);
}

// ── 4. 주소 수정 (공사 중 껍데기) ─────────────────────────
void AddressHandler::handleAddressUpdate(std::shared_ptr<ClientSession> session, const std::string& jsonBody) {
    std::cout << "[AddressHandler] Update API 호출됨 (현재 구현 중)" << std::endl;
    nlohmann::json res;
    res["status"] = 501; 
    session->sendPacket(static_cast<uint16_t>(CmdID::RES_ADDRESS_UPDATE), res);
}

// ── 5. 기본 주소 설정 (공사 중 껍데기) ──────────────────────
void AddressHandler::handleAddressDefault(std::shared_ptr<ClientSession> session, const std::string& jsonBody) {
    std::cout << "[AddressHandler] Default API 호출됨 (현재 구현 중)" << std::endl;
    nlohmann::json res;
    res["status"] = 501; 
    session->sendPacket(static_cast<uint16_t>(CmdID::RES_ADDRESS_DEFAULT), res);
}