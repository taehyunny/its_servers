#include "Dispatcher.h"
#include "AllDTOs.h"
#include "UserHandler.h"
#include "OrderHandler.h"
#include "SystemHandler.h"
#include "StoreHandler.h"
#include <iostream>

void Dispatcher::dispatch(ClientSession *session, const PacketHeader &header, const std::string &jsonBody, ThreadPool &pool)
{ // 디버깅용 로그: 수신된 CmdID 출력
    std::cout << "[Dispatcher] 수신된 CmdID: " << static_cast<int>(header.cmdId) << std::endl;

    switch (header.cmdId)
    {
    case CmdID::REQ_SIGNUP:
        // 🚀 수정: 주입받은 지역 변수 pool을 사용합니다.
        pool.enqueue([session, jsonBody]()
                     { UserHandler::handleSignup(session, jsonBody); }); // 람다 캡처로 session과 jsonBody를 넘겨줍니다.
        break;
    case CmdID::REQ_LOGIN:
        pool.enqueue([session, jsonBody]()
                     { UserHandler::handleLogin(session, jsonBody); }); // 로그인 요청도 UserHandler에서 처리하도록 합니다.
        break;
    case CmdID::REQ_STORE_LIST:
        pool.enqueue([session, jsonBody]()
                     { StoreHandler::handleStoreListRequest(session, jsonBody); });
        break;

        // case CmdID::REQ_STORE_LIST:
        // case CmdID::REQ_ORDER_CREATE:
        //     g_threadPool.enqueue([session, header, jsonBody]()
        //                          { OrderHandler::handleOrder(session, header.cmdId, jsonBody); });
        //     break;

    default:
        std::cerr << "[WARNING] 알 수 없는 명령어: " << static_cast<int>(header.cmdId) << std::endl;
        break;
    }
}