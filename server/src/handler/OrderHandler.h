#pragma once
#include <memory>
#include <string>

class ClientSession; // 전방 선언

class OrderHandler
{
public:
    // 🚀 장바구니 결제 요청 처리기

    static void handleOrderAccept(std::shared_ptr<ClientSession> session, const std::string &jsonBody); // 주문 수락 및 조리 시작 요청 (3000)
    static void handleCheckoutInfo(std::shared_ptr<ClientSession> session, const std::string &jsonBody);
    static void handleChangeOrderState(std::shared_ptr<ClientSession> session, const std::string &jsonBody); // 주문 상태 변경 (3002)
    static void handleOrderList(std::shared_ptr<ClientSession> session, const std::string &jsonBody);
    static void handleOrderReject(std::shared_ptr<ClientSession> session, const std::string &jsonBody);        // 🚀 사장님 주문 거절 기능 (3004)
    static void handleOrderHistory(std::shared_ptr<ClientSession> session, const std::string &jsonBody);       // 주문 내역 조회 (고객용) (3080)
    static void handleOrderHistorySearch(std::shared_ptr<ClientSession> session, const std::string &jsonBody); // 주문 내역 검색 (고객용) (3083)
    static void handleCreateOrder(std::shared_ptr<ClientSession> session, const std::string &jsonBody);        // 테스트용 주문 생성 (3100)
    static void handleCookTimeSet(std::shared_ptr<ClientSession> session, const std::string &jsonBody);        // 조리 시간 설정 요청 (3020)
    static void handleDeliveryComplete(std::shared_ptr<ClientSession> session, const std::string &jsonBody);   // 배달 완료 보고 (4010)
    static void handleCancel(std::shared_ptr<ClientSession> session, const std::string &jsonBody);             // 주문 취소 요청 (고객용) (3090)
    static void handleOrderDetail(std::shared_ptr<ClientSession> session, const std::string &jsonBody);        // 주문 상세 조회 (고객용) (3085)
};