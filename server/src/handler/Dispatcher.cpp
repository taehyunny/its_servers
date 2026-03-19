#include "Dispatcher.h"
#include "AllDTOs.h"
#include "UserHandler.h"
#include "OrderHandler.h"
#include "SystemHandler.h"
#include "StoreHandler.h"
#include "MenuHandler.h"
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
        pool.enqueue([session, jsonBody]() {  // DB 조회가 일어나므로 스레드 풀(pool)에게 일을 던집니다.
            try {
                // 🚀 핵심: 문자열(jsonBody)을 JSON 객체로 예쁘게 포장해서 넘겨야 합니다!
                nlohmann::json jsonPayload = nlohmann::json::parse(jsonBody);// JSON 파싱이 실패할 경우 예외가 발생할 수 있으므로 try-catch로 감싸줍니다.
                StoreHandler::handleStoreListRequest(session, jsonPayload);  // StoreHandler의 handleStoreListRequest 함수는 이제 JSON 객체를 받도록 시그니처가 바뀌었으므로, jsonPayload를 넘겨줍니다.
            } catch (const std::exception& e) {
                std::cerr << "[Dispatcher] 상점 목록 JSON 파싱 에러: " << e.what() << std::endl;
            }
        });
        break;
    case CmdID::REQ_MENU_LIST:
        // DB 조회가 일어나므로 스레드 풀(pool)에게 일을 던집니다.
        pool.enqueue([session, jsonBody]() {
            try {
                nlohmann::json jsonPayload = nlohmann::json::parse(jsonBody); // JSON 파싱이 실패할 경우 예외가 발생할 수 있으므로 try-catch로 감싸줍니다.
                MenuHandler::handleMenuListRequest(session, jsonPayload);  // MenuHandler의 handleMenuListRequest 함수는 이제 JSON 객체를 받도록 시그니처가 바뀌었으므로, jsonPayload를 넘겨줍니다.
            } catch (const std::exception& e) {
                std::cerr << "[Dispatcher] 메뉴 요청 JSON 파싱 에러: " << e.what() << std::endl; // 파싱 에러 로그 추가
            }
        });
        break;

    default:
        std::cerr << "[WARNING] 알 수 없는 명령어: " << static_cast<int>(header.cmdId) << std::endl;
        break;
    }
}