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
                UserHandler::handleSignup(session.get(), jsonBody);
            } catch (const std::exception& e) { 
                std::cerr << "🚨 [Dispatcher-Signup] 에러: " << e.what() << std::endl;  
            } });
        break;

    case CmdID::REQ_LOGIN: // 로그인 요청
        pool.enqueue([session, jsonBody]()
                     {
            try {
                UserHandler::handleLogin(session.get(), jsonBody); // 로그인은 JSON 파싱이 내부에서 일어나므로 예외 처리를 핸들러 내부로 이동
            } catch (const std::exception& e) {
                std::cerr << "🚨 [Dispatcher-Login] 에러: " << e.what() << std::endl;
            } });
        break;

    case CmdID::REQ_STORE_LIST:
        pool.enqueue([session, jsonBody]()
                     {
            try {
                nlohmann::json jsonPayload = nlohmann::json::parse(jsonBody);  // JSON 파싱을 여기서 시도하여 에러를 잡아냅니다.
                StoreHandler::handleStoreListRequest(session.get(), jsonPayload);    // 매장 목록 요청은 JSON 파싱이 필요하므로 예외 처리를 추가합니다.
            } catch (const std::exception &e) {
                std::cerr << "🚨 [Dispatcher-StoreList] JSON 파싱 에러: " << e.what() << std::endl;
            } });
        break;

    case CmdID::REQ_MENU_LIST:
        pool.enqueue([session, jsonBody]()
                     {
            try {
                nlohmann::json jsonPayload = nlohmann::json::parse(jsonBody);  // JSON 파싱을 여기서 시도하여 에러를 잡아냅니다.
                MenuHandler::handleMenuListRequest(session.get(), jsonPayload);     // 메뉴 목록 요청은 JSON 파싱이 필요하므로 예외 처리를 추가합니다.
            } catch (const std::exception& e) {
                std::cerr << "🚨 [Dispatcher-MenuList] JSON 파싱 에러: " << e.what() << std::endl;
            } });
        break;

    case CmdID::REQ_CATEGORY: // 카테고리 요청은 JSON 파싱이 필요 없으므로 핸들러 내부에서 예외 처리를 하지 않고 바로 호출합니다.
        pool.enqueue([session, jsonBody]()
                     {
            // 카테고리는 파싱 없이 문자열만 던지므로 비교적 안전합니다.
            CategoryHandler::handleCategoryRequest(session.get(), jsonBody); });
        break;

    case CmdID::REQ_AUTH_CHECK: // 아이디 중복 확인 요청
        pool.enqueue([session, jsonBody]()
                     {
            try {
                UserHandler::handleAuthCheck(session.get(), jsonBody);  /// 아이디 중복 확인도 JSON 파싱이 필요하므로 예외 처리를 추가합니다.
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