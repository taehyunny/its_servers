#include "Dispatcher.h"
#include "UserHandler.h"
#include "OrderHandler.h"
#include "StoreHandler.h"
#include "MenuHandler.h"
#include "CategoryHandler.h"
#include "SearchHandler.h"
#include "AddressHandler.h"
#include "ReviewHandler.h"
#include "SalesHandler.h"  // 🚀 주석 해제 및 활성화!
#include "ChatHandler.h"   // 🚀 신규: 채팅 상담 핸들러
#include "RiderHandler.h"  // 🚀 신규: 라이더 관련 핸들러
#include "AdminHandler.h"  // 🚀 신규: 관리자 관련 핸들러
#include "SystemHandler.h" // 🚀 신규: 시스템 관련 핸들러
#include <iostream>

// =========================================================
// 🌐 핸들러 맵 초기화 (도메인별 구역 분리)
// =========================================================
const std::unordered_map<CmdID, Dispatcher::HandlerFunc> Dispatcher::_handlerMap = {

    // ㅡ ㅡ 0. 공통 (Common) ㅡ ㅡ
    {CmdID::REQ_HEARTBEAT, [](auto s, auto b)
     { SystemHandler::handleHeartbeat(s, b); }},

    // ── 1. 유저 및 인증 (Auth & User) ──
    {CmdID::REQ_SIGNUP, [](auto s, auto b)
     { UserHandler::handleSignup(s, b); }},
    {CmdID::REQ_LOGIN, [](auto s, auto b)
     { UserHandler::handleLogin(s, b); }},
    {CmdID::REQ_LOGOUT, [](auto s, auto b)
     { UserHandler::handleLogout(s, b); }}, // 🚀 신규: 로그아웃
    {CmdID::REQ_AUTH_CHECK, [](auto s, auto b)
     { UserHandler::handleAuthCheck(s, b); }},
    {CmdID::REQ_PHONE_CHECK, [](auto s, auto b)
     { UserHandler::handlePhoneCheck(s, b); }},
    {CmdID::REQ_BUISNESS_NUM_CHECK, [](auto s, auto b)
     { UserHandler::handleBizNumCheck(s, b); }},

    // ── 2. 매장 및 메뉴 (Store & Menu) ──
    {CmdID::REQ_ORDER_LIST, [](auto s, auto b)
     { OrderHandler::handleOrderList(s, b); }},
    {CmdID::REQ_STORE_LIST, [](auto s, auto b)
     { StoreHandler::handleStoreListRequest(s, b); }},
    {CmdID::REQ_STORE_DETAIL, [](auto s, auto b)
     { StoreHandler::handleStoreDetailReq(s, b); }},
    {CmdID::REQ_MENU_LIST, [](auto s, auto b)
     { MenuHandler::handleMenuListRequest(s, b); }},
    {CmdID::REQ_STORE_INFO_UPDATE, [](auto s, auto b)
     { StoreHandler::handleStoreInfoUpdateReq(s, b); }},
    {CmdID::REQ_STORE_STATUS_SET, [](auto s, auto b)
     { StoreHandler::handleStoreStatusSet(s, b); }},
    {CmdID::REQ_MENU_EDIT, [](auto s, auto b)
     { MenuHandler::handleMenuEdit(s, b); }},
    {CmdID::REQ_MENU_SOLD_OUT, [](auto s, auto b)
     { MenuHandler::handleMenuSoldOut(s, b); }},

    // ── 3. 주문 및 결제 (Order & Payment) ──
    {CmdID::REQ_ORDER_REJECT, [](auto s, auto b)
     { OrderHandler::handleOrderReject(s, b); }},
    {CmdID::REQ_CHECKOUT_INFO, [](auto s, auto b)
     { OrderHandler::handleCheckoutInfo(s, b); }},
    {CmdID::REQ_ORDER_CREATE, [](auto s, auto b)
     { OrderHandler::handleCreateOrder(s, b); }},
    {CmdID::REQ_ORDER_ACCEPT, [](auto s, auto b)
     { OrderHandler::handleOrderAccept(s, b); }},
    {CmdID::REQ_MENU_OPTION, [](auto s, auto b)
     { MenuHandler::handleMenuOption(s, b); }},
    {CmdID::REQ_CHANGE_ORDER_STATE, [](auto s, auto b)
     { OrderHandler::handleChangeOrderState(s, b); }},
    {CmdID::REQ_ORDER_REJECT, [](auto s, auto b)
     { OrderHandler::handleOrderReject(s, b); }}, // 🚀 사장님 주문 거절 기능 (구현되어 있다면 활성화)
    {CmdID::REQ_ORDER_HISTORY, [](auto s, auto b)
     { OrderHandler::handleOrderHistory(s, b); }}, // 주문 내역 조회 (고객용)
    {CmdID::REQ_ORDER_HISTORY_SEARCH, [](auto s, auto b)
     { OrderHandler::handleOrderHistorySearch(s, b); }}, // 주문 내역 검색 (고객용)
    {CmdID::REQ_COOK_TIME_SET, [](auto s, auto b)
     { OrderHandler::handleCookTimeSet(s, b); }}, // 조리 시간 설정 요청 (사장님용)
    {CmdID::REQ_PICKUP, [](auto s, auto b)
     { RiderHandler::handlePickup(s, b); }}, // 픽업 완료 요청 (라이더용)
    {CmdID::REQ_ORDER_DETAIL, [](auto s, auto b)
     { OrderHandler::handleOrderDetail(s, b); }}, // 주문 상세 조회 (고객용) (3085)

    // ── 4. 리뷰 및 통계 (Review & Sales) ──
    {CmdID::REQ_REVIEW_LIST, [](auto s, auto b)
     { ReviewHandler::handleReviewList(s, b); }},
    {CmdID::REQ_REVIEW_REPLY, [](auto s, auto b)
     { ReviewHandler::handleReviewReply(s, b); }},
    {CmdID::REQ_SALES_STAT, [](auto s, auto b)
     { SalesHandler::handleSalesStat(s, b); }}, // 🚀 신규: 매출 통계
    {CmdID::REQ_REVIEW_LIST, [](auto s, auto b)
     { ReviewHandler::handleReviewList(s, b); }}, // 리뷰 목록 요청 (고객용, 매장 상세 화면에서)
    {CmdID::REQ_MENU_REVIEW_LIST, [](auto s, auto b)
     { ReviewHandler::handleMenuReviewList(s, b); }}, // 특정 메뉴 리뷰 목록 요청 (고객용, 메뉴 상세 화면에서)

    // ── 5. 주소 관리 (Address) ──
    {CmdID::REQ_ADDRESS_SAVE, [](auto s, auto b)
     { AddressHandler::handleAddressSave(s, b); }},
    {CmdID::REQ_ADDRESS_LIST, [](auto s, auto b)
     { AddressHandler::handleAddressList(s, b); }},
    {CmdID::REQ_ADDRESS_DELETE, [](auto s, auto b)
     { AddressHandler::handleAddressDelete(s, b); }},
    {CmdID::REQ_ADDRESS_UPDATE, [](auto s, auto b)
     { AddressHandler::handleAddressUpdate(s, b); }},
    {CmdID::REQ_ADDRESS_DEFAULT, [](auto s, auto b)
     { AddressHandler::handleAddressDefault(s, b); }},

    // ── 6. 검색 (Search) ──
    {CmdID::REQ_RESEARCH_WIDGET, [](auto s, auto b)
     { SearchHandler::handleSearchWidgetReq(s, b); }},
    {CmdID::REQ_RESEARCH_ADD, [](auto s, auto b)
     { SearchHandler::handleSearchAddReq(s, b); }},
    {CmdID::REQ_RESEARCH_DELETE, [](auto s, auto b)
     { SearchHandler::handleSearchDeleteReq(s, b); }},
    {CmdID::REQ_RESEARCH_DEL_ALL, [](auto s, auto b)
     { SearchHandler::handleSearchDelAllReq(s, b); }},
    {CmdID::REQ_SEARCH_STORE, [](auto s, auto b)
     { SearchHandler::handleSearchStoreReq(s, b); }},
    // ── 7. 마이페이지 등 기타 (MyPage & Misc) ──
    {CmdID::REQ_GRADE_UPDATE, [](auto s, auto b)
     { UserHandler::handleGradeUpdate(s, b); }}, // 🚀 신규: 등급 업그레이드 요청 (고객용)

    // ── 8. 1:1 채팅 상담 (Chat) ──
    {CmdID::REQ_CHAT_CONNECT, [](auto s, auto b)
     { ChatHandler::handleChatRequest(s, b); }},
    {CmdID::REQ_CHAT_SEND, [](auto s, auto b)
     { ChatHandler::handleChatSend(s, b); }},
    {CmdID::REQ_CHAT_CLOSE, [](auto s, auto b)
     { ChatHandler::handleChatClose(s, b); }}, // 🚀 신규: 채팅 종료 요청 (관리자 또는 사장님이 채팅 종료를 요청했을 때)

    // ── 9. 라이더 관련 (Rider) ──
    {CmdID::REQ_RIDER_ORDER_LIST, [](auto s, auto b)
     { RiderHandler::handleRiderOrderList(s, b); }},
    {CmdID::REQ_DELIVERY_COMPLETE, [](auto s, auto b)
     { RiderHandler::handleDeliveryComplete(s, b); }},

    // ── 10. 관리자 관련 (Admin) ──
    {CmdID::REQ_ADMIN_INIT, [](auto s, auto b)
     { AdminHandler::handleAdminInit(s, b); }},
    {CmdID::REQ_ADMIN_ORDER_LIST, [](auto s, auto b)
     { AdminHandler::handleAdminOrderList(s, b); }},
    {CmdID::RES_REQUEST_OK, [](auto s, auto b)
     { ChatHandler::handleChatAccept(s, b); }},
    {CmdID::REQ_CANCEL, [](auto s, auto b)
     { OrderHandler::handleCancel(s, b); }},

};
// =========================================================
// 🚀 [2단계] Dispatch 함수 본체
// =========================================================
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