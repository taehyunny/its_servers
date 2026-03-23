#include "Dispatcher.h"
#include "UserHandler.h"
#include "OrderHandler.h"
#include "SystemHandler.h"
#include "StoreHandler.h"
#include "MenuHandler.h"
#include "CategoryHandler.h"
#include "SearchHandler.h"
#include <iostream>

// 핸들러 맵 초기화
const std::unordered_map<CmdID, Dispatcher::HandlerFunc> Dispatcher::_handlerMap = {
    {CmdID::REQ_SIGNUP, [](auto s, auto b)
     { UserHandler::handleSignup(s, b); }},
    {CmdID::REQ_LOGIN, [](auto s, auto b)
     { UserHandler::handleLogin(s, b); }},
    {CmdID::REQ_STORE_LIST, [](auto s, auto b)
     { StoreHandler::handleStoreListRequest(s, b); }},
    {CmdID::REQ_MENU_LIST, [](auto s, auto b)
     { MenuHandler::handleMenuListRequest(s, b); }},
    {CmdID::REQ_AUTH_CHECK, [](auto s, auto b)
     { UserHandler::handleAuthCheck(s, b); }},
    {CmdID::REQ_PHONE_CHECK, [](auto s, auto b)
     { UserHandler::handlePhoneCheck(s, b); }},
    {CmdID::REQ_BUISNESS_NUM_CHECK, [](auto s, auto b)
     { UserHandler::handleBizNumCheck(s, b); }},
    {CmdID::REQ_ORDER_CREATE, [](auto s, auto b)
     { OrderHandler::handleOrderCreate(s, b); }},
    {CmdID::REQ_MENU_LIST, [](auto s, auto b)
     { MenuHandler::handleMenuListRequest(s, b); }},
    {CmdID::REQ_ORDER_CREATE, [](auto s, auto b)
     { OrderHandler::handleOrderCreate(s, b); }},
    {CmdID::REQ_ORDER_ACCEPT, [](auto s, auto b)
     { OrderHandler::handleOrderAccept(s, b); }},
    {CmdID::REQ_RESEACH_WIDGET, [](auto s, auto b)
     { SearchHandler::handleSearchWidgetReq(s, b); }},
    {CmdID::REQ_RESEARCH_DELETE, [](auto s, auto b)
     { SearchHandler::handleSearchDeleteReq(s, b); }},
    {CmdID::REQ_RESEARCH_ADD, [](auto s, auto b)
     { SearchHandler::handleSearchAddReq(s, b); }},
    {CmdID::REQ_RESEARCH_DEL_ALL, [](auto s, auto b)
     { SearchHandler::handleSearchDelAllReq(s, b); }},
    {CmdID::REQ_SEARCH_STORE, [](auto s, auto b)
     { SearchHandler::handleSearchStoreReq(s, b); }},
    {CmdID::REQ_RESEARCH_ADD, [](auto s, auto b)
     { SearchHandler::handleSearchAddReq(s, b); }},
    {CmdID::REQ_STORE_INFO_UPDATE, [](auto s, auto b)
     { StoreHandler::handleStoreInfoUpdateReq(s, b); }},
    {CmdID::REQ_STORE_STATUS_SET, [](auto s, auto b)
     { StoreHandler::handleStoreStatusSet(s, b); }}

    // ❌ 주문 거절 (3010) - 나중에 구현할 때 대비해서 미리 등록!
    // {CmdID::REQ_ORDER_REJECT, [](auto s, auto b)
    //     { OrderHandler::handleOrderReject(s, b); }}

};

// 🚀 [2단계] Dispatch 함수 본체 (반복되던 스레드 풀과 try-catch를 하나로 통합!)
void Dispatcher::dispatch(std::shared_ptr<ClientSession> session, const PacketHeader &header, const std::string &jsonBody, ThreadPool &pool)
{
    std::cout << "\n==================================================" << std::endl;
    std::cout << "[Dispatcher] 📥 수신된 CmdID: " << static_cast<int>(header.cmdId) << std::endl;
    std::cout << "[Dispatcher] 📦 Payload: " << (jsonBody.empty() ? "(empty)" : jsonBody) << std::endl;
    std::cout << "==================================================\n"
              << std::endl;

    // 맵에서 명령어 찾기
    auto it = _handlerMap.find(header.cmdId);

    if (it != _handlerMap.end())
    {
        auto handler = it->second;                        // 실행할 함수 추출
        int cmdIdForLog = static_cast<int>(header.cmdId); // 로그용 ID

        // 스레드 풀에 작업 할당 (여기서 단 한 번만 작성!)
        pool.enqueue([handler, session, jsonBody, cmdIdForLog]()
                     {
            try {
                handler(session, jsonBody); // 실제 핸들러 실행
            } catch (const std::exception& e) {
                std::cerr << "🚨 [Dispatcher-CmdID: " << cmdIdForLog << "] 에러: " << e.what() << std::endl;
            } });
    }
    else
    {
        std::cerr << "⚠️ [WARNING] 등록되지 않은 알 수 없는 명령어 수신: " << static_cast<int>(header.cmdId) << std::endl;
    }
}