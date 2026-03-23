#pragma once
#include <memory>
#include <string>

class ClientSession;

class AddressHandler {
public:
    static void handleAddressSave(std::shared_ptr<ClientSession> session, const std::string& jsonBody);
    static void handleAddressList(std::shared_ptr<ClientSession> session, const std::string& jsonBody);
    static void handleAddressDelete(std::shared_ptr<ClientSession> session, const std::string& jsonBody);
    static void handleAddressUpdate(std::shared_ptr<ClientSession> session, const std::string& jsonBody);
    static void handleAddressDefault(std::shared_ptr<ClientSession> session, const std::string& jsonBody);
};


