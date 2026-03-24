#include "AddressHandler.h"
#include "AddressDAO.h"
#include "ClientSession.h"
#include "Global_protocol.h"
#include "AddressDTO.h"
#include <nlohmann/json.hpp>
#include <iostream>

using nlohmann::json;

// ── 1. 주소 저장 ──────────────────────────────
void AddressHandler::handleAddressSave(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        auto req = json::parse(jsonBody).get<ReqAddressSaveDTO>();
        int newAddressId = AddressDAO::getInstance().saveAddress(req);

        ResAddressSaveDTO res;
        res.status = (newAddressId > 0) ? 200 : 500;
        res.addressId = newAddressId;

        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ADDRESS_SAVE), json(res));
    }
    catch (const std::exception &e)
    {
        std::cerr << "[AddressHandler] Save Error: " << e.what() << std::endl;
    }
}

// ── 2. 주소 목록 조회 ──────────────────────────
void AddressHandler::handleAddressList(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        auto req = json::parse(jsonBody).get<ReqAddressListDTO>();
        auto addresses = AddressDAO::getInstance().getAddressList(req.userId);

        ResAddressListDTO res;
        res.status = 200;
        res.addresses = addresses;

        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ADDRESS_LIST), json(res));
    }
    catch (const std::exception &e)
    {
        std::cerr << "[AddressHandler] List Error: " << e.what() << std::endl;
    }
}

// ── 3. 주소 삭제 (🚀 501 제거 및 로직 구현) ────────────────
void AddressHandler::handleAddressDelete(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        auto req = json::parse(jsonBody).get<ReqAddressDeleteDTO>();
        // DAO 호출: 기본 주소 삭제 방지 로직이 DAO 내부에 들어있음
        bool success = AddressDAO::getInstance().deleteAddress(req.userId, req.addressId);

        ResAddressDeleteDTO res;
        res.status = success ? 200 : 400; // 실패 시 400 반환 (지나님 요청사항)

        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ADDRESS_DELETE), json(res));
    }
    catch (const std::exception &e)
    {
        std::cerr << "[AddressHandler] Delete Error: " << e.what() << std::endl;
    }
}

// ── 4. 주소 수정 (🚀 501 제거 및 로직 구현) ────────────────
void AddressHandler::handleAddressUpdate(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        auto req = json::parse(jsonBody).get<ReqAddressUpdateDTO>();
        bool success = AddressDAO::getInstance().updateAddress(req);

        ResAddressUpdateDTO res;
        res.status = success ? 200 : 400;

        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ADDRESS_UPDATE), json(res));
    }
    catch (const std::exception &e)
    {
        std::cerr << "[AddressHandler] Update Error: " << e.what() << std::endl;
    }
}

// ── 5. 기본 주소 변경 (🚀 501 제거 및 로직 구현) ──────────────
void AddressHandler::handleAddressDefault(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        auto req = json::parse(jsonBody).get<ReqAddressDefaultDTO>();
        bool success = AddressDAO::getInstance().setDefaultAddress(req.userId, req.addressId);

        ResAddressDefaultDTO res;
        res.status = success ? 200 : 400;

        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ADDRESS_DEFAULT), json(res));
    }
    catch (const std::exception &e)
    {
        std::cerr << "[AddressHandler] Default Error: " << e.what() << std::endl;
    }
}