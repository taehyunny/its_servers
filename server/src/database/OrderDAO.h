#pragma once
#include <string>
#include <vector>
#include <memory>
#include "AllDTOs.h" // 태현님의 완벽한 OrderCreateReqDTO가 있는 곳

// 반환용 구조체 (성공 여부, 메시지, 생성된 주문번호)
struct OrderResult
{
    bool isSuccess;
    std::string message;
    std::string orderId;
};

class OrderDAO
{
public:
    static OrderDAO &getInstance()
    {
        static OrderDAO instance;
        return instance;
    }

    // 🚀 핵심: 클라이언트의 요청(DTO)을 받아 DB에 꽂아넣는 함수
    OrderResult createOrder(const OrderCreateReqDTO &req);

    bool updateOrderStatus(const std::string &orderId, int status);

    // 🚀 주문번호로 고객 ID 찾아오기 (알림 대상 식별용)
    std::string getCustomerIdByOrderId(const std::string &orderId);
    std::vector<OrderHistoryItemDTO> getOrderHistory(const std::string &userId, const std::string &keyword);

private:
    OrderDAO() = default;
    ~OrderDAO() = default;
    OrderDAO(const OrderDAO &) = delete;
    OrderDAO &operator=(const OrderDAO &) = delete;
};