#include "AddressHandler.h"
#include "AddressDAO.h"
#include "ClientSession.h"
#include "Global_protocol.h"
#include "AddressDTO.h"
#include <nlohmann/json.hpp>
#include <iostream>

using nlohmann::json;

// ── 1. 주소 저장 (추가) ──────────────────────────────
void AddressHandler::handleAddressSave(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        auto req = json::parse(jsonBody).get<ReqAddressSaveDTO>();

        // 🚀 방금 AddressDAO.cpp에 작성한 saveAddress 함수를 호출합니다!
        int newAddressId = AddressDAO::getInstance().saveAddress(req);

        ResAddressSaveDTO res;
        if (newAddressId > 0)
        {
            res.status = 200;
            res.addressId = newAddressId;
            session->sendPacket(static_cast<uint16_t>(CmdID::RES_ADDRESS_SAVE), json(res));

            // 🚀 [오토 리프레시]: 추가 완료 후 최신 리스트 푸시!
            auto addresses = AddressDAO::getInstance().getAddressList(req.userId);
            ResAddressListDTO listRes;
            listRes.status = 200;
            listRes.addresses = addresses;
            session->sendPacket(static_cast<uint16_t>(CmdID::RES_ADDRESS_LIST), json(listRes));
            std::cout << "[AddressHandler] 주소 저장 및 실시간 갱신 완료!" << std::endl;
        }
        else
        {
            res.status = 500;
            res.addressId = 0;
            session->sendPacket(static_cast<uint16_t>(CmdID::RES_ADDRESS_SAVE), json(res));
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [AddressHandler] Save Error: " << e.what() << std::endl;
        ResAddressSaveDTO res;
        res.status = 500;
        res.addressId = 0;
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ADDRESS_SAVE), json(res)); // 무한 로딩 방어
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
        std::cerr << "🚨 [AddressHandler] List Error: " << e.what() << std::endl;
        ResAddressListDTO res;
        res.status = 500;
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ADDRESS_LIST), json(res)); // 무한 로딩 방어
    }
}

// ── 3. 주소 삭제 ────────────────
void AddressHandler::handleAddressDelete(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        auto req = json::parse(jsonBody).get<ReqAddressDeleteDTO>();
        bool success = AddressDAO::getInstance().deleteAddress(req.userId, req.addressId);

        ResAddressDeleteDTO res;
        res.status = success ? 200 : 400;
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ADDRESS_DELETE), json(res));

        // 🚀 [오토 리프레시]: 삭제 성공 시 최신 리스트 푸시!
        if (success)
        {
            auto addresses = AddressDAO::getInstance().getAddressList(req.userId);
            ResAddressListDTO listRes;
            listRes.status = 200;
            listRes.addresses = addresses;
            session->sendPacket(static_cast<uint16_t>(CmdID::RES_ADDRESS_LIST), json(listRes));
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [AddressHandler] Delete Error: " << e.what() << std::endl;
        ResAddressDeleteDTO res;
        res.status = 500;
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ADDRESS_DELETE), json(res)); // 무한 로딩 방어
    }
}

// ── 4. 주소 수정 ────────────────
void AddressHandler::handleAddressUpdate(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        auto req = json::parse(jsonBody).get<ReqAddressUpdateDTO>();
        bool success = AddressDAO::getInstance().updateAddress(req);

        ResAddressUpdateDTO res;
        res.status = success ? 200 : 400;
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ADDRESS_UPDATE), json(res));

        // 🚀 [오토 리프레시]: 수정 성공 시 최신 리스트 푸시!
        if (success)
        {
            auto addresses = AddressDAO::getInstance().getAddressList(req.userId);
            ResAddressListDTO listRes;
            listRes.status = 200;
            listRes.addresses = addresses;
            session->sendPacket(static_cast<uint16_t>(CmdID::RES_ADDRESS_LIST), json(listRes));
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [AddressHandler] Update Error: " << e.what() << std::endl;
        ResAddressUpdateDTO res;
        res.status = 500;
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ADDRESS_UPDATE), json(res)); // 무한 로딩 방어
    }
}

// ── 5. 기본 주소 변경 ──────────────
void AddressHandler::handleAddressDefault(std::shared_ptr<ClientSession> session, const std::string &jsonBody)
{
    try
    {
        auto req = json::parse(jsonBody).get<ReqAddressDefaultDTO>();
        bool success = AddressDAO::getInstance().setDefaultAddress(req.userId, req.addressId);

        ResAddressDefaultDTO res;
        res.status = success ? 200 : 400;
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ADDRESS_DEFAULT), json(res));

        // 🚀 [오토 리프레시]: 기본 주소 변경 성공 시 최신 리스트 푸시!
        if (success)
        {
            auto addresses = AddressDAO::getInstance().getAddressList(req.userId);
            ResAddressListDTO listRes;
            listRes.status = 200;
            listRes.addresses = addresses;
            session->sendPacket(static_cast<uint16_t>(CmdID::RES_ADDRESS_LIST), json(listRes));
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "🚨 [AddressHandler] Default Error: " << e.what() << std::endl;
        ResAddressDefaultDTO res;
        res.status = 500;
        session->sendPacket(static_cast<uint16_t>(CmdID::RES_ADDRESS_DEFAULT), json(res)); // 무한 로딩 방어
    }
}