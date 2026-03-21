#pragma once
#include <string>
#include "ClientSession.h" // ClientSession 참조를 위해 필요

class UserHandler
{
public:
    static void handleSignup(ClientSession *session, const std::string &jsonBody);
    static void handleLogin(ClientSession *session, const std::string &jsonBody);
    static void handleAuthCheck(ClientSession *session, const std::string &jsonBody);
    static void handlePhoneCheck(ClientSession *session, const std::string &jsonBody);
    static void handleBizNumCheck(std::shared_ptr<ClientSession> session, const std::string &jsonBody);
};