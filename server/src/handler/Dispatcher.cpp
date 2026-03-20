#include "Dispatcher.h"
#include "AllDTOs.h"
#include "UserHandler.h"
#include "OrderHandler.h"
#include "SystemHandler.h"
#include "StoreHandler.h"
#include "MenuHandler.h"
#include "CategoryHandler.h"
#include <iostream>

void Dispatcher::dispatch(std::shared_ptr<ClientSession> session, const PacketHeader &header, const std::string &jsonBody, ThreadPool &pool)
{
    // 🚀 [핵심 로그] 어떤 세션에서 어떤 명령어가, 어떤 데이터와 함께 들어왔는지 최전선에서 기록!
    std::cout << "\n==================================================" << std::endl;
    std::cout << "[Dispatcher] 📥 수신된 CmdID: " << static_cast<int>(header.cmdId) << std::endl;
    std::cout << "[Dispatcher] 📦 Payload: " << (jsonBody.empty() ? "(empty)" : jsonBody) << std::endl;
    std::cout << "==================================================\n"
              << std::endl;

    switch (header.cmdId)
    {
    case CmdID::REQ_SIGNUP: // 회원가입 요청
        pool.enqueue([session, jsonBody]()
                     {
            try {
                // ⚠️ UserHandler는 아직 리팩토링 전이므로 .get() 유지
                UserHandler::handleSignup(session.get(), jsonBody); 
            } catch (const std::exception& e) { 
                std::cerr << "🚨 [Dispatcher-Signup] 에러: " << e.what() << std::endl;  
            } });
        break;

    case CmdID::REQ_LOGIN: // 로그인 요청
        pool.enqueue([session, jsonBody]()
                     {
            try {
                UserHandler::handleLogin(session.get(), jsonBody); 
            } catch (const std::exception& e) {
                std::cerr << "🚨 [Dispatcher-Login] 에러: " << e.what() << std::endl;
            } });
        break;

    case CmdID::REQ_STORE_LIST: // 카테고리별 매장 목록 요청
        pool.enqueue([session, jsonBody]()
                     {
            try {
                // 🚀 우리가 완벽하게 리팩토링한 StoreHandler! (session 그대로 전달, 파싱도 내부에서)
                StoreHandler::handleStoreListRequest(session, jsonBody);    
            } catch (const std::exception &e) {
                std::cerr << "🚨 [Dispatcher-StoreList] 에러: " << e.what() << std::endl;
            } });
        break;

    case CmdID::REQ_MENU_LIST: // 매장별 메뉴 목록 요청
        pool.enqueue([session, jsonBody]()
                     {
            try {
                // ⚠️ MenuHandler는 아직 리팩토링 전이므로 기존 로직 유지
                nlohmann::json jsonPayload = nlohmann::json::parse(jsonBody);  
                MenuHandler::handleMenuListRequest(session.get(), jsonPayload);     
            } catch (const std::exception& e) {
                std::cerr << "🚨 [Dispatcher-MenuList] JSON 파싱 에러: " << e.what() << std::endl;
            } });
        break;

    case CmdID::REQ_AUTH_CHECK: // 아이디 중복 확인 요청
        pool.enqueue([session, jsonBody]()
                     {
            try {
                UserHandler::handleAuthCheck(session.get(), jsonBody);  
            } catch (const std::exception& e) {
                std::cerr << "🚨 [Dispatcher-AuthCheck] 에러: " << e.what() << std::endl;
            } });
        break;

    case CmdID::REQ_PHONE_CHECK: // 전화번호 중복 확인 요청
        pool.enqueue([session, jsonBody]()
                     {
            try {
                UserHandler::handlePhoneCheck(session.get(), jsonBody);
            } catch (const std::exception& e) {
                std::cerr << "🚨 [Dispatcher-PhoneCheck] 에러: " << e.what() << std::endl;
            } });
        break;

    default:
        std::cerr << "⚠️ [WARNING] 알 수 없는 명령어 수신: " << static_cast<int>(header.cmdId) << std::endl;
        break;
    }
}