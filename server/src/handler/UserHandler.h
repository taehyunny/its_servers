#pragma once
#include <memory>
#include <string>
#include "ClientSession.h" // ClientSession 참조를 위해 필요

class ClientSession;

class UserHandler
{
public:
    static void handleSignup(std::shared_ptr<ClientSession> session, const std::string &jsonBody);
    static void handleLogin(std::shared_ptr<ClientSession> session, const std::string &jsonBody);
    static void handleAuthCheck(std::shared_ptr<ClientSession> session, const std::string &jsonBody);
    static void handlePhoneCheck(std::shared_ptr<ClientSession> session, const std::string &jsonBody);
    static void handleBizNumCheck(std::shared_ptr<ClientSession> session, const std::string &jsonBody);
    static void handleLogout(std::shared_ptr<ClientSession> session, const std::string &jsonBody);
    static void handleGradeUpdate(std::shared_ptr<ClientSession> session, const std::string &jsonBody);
};