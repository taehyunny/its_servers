#pragma once
#include <string>
#include <vector>
#include "json.hpp"

struct OrderItemDTO {  // 주문 아이템 하나를 나타내는 DTO
    int menuId;        // 주문한 메뉴의 ID
    int quantity;      // 주문한 수량
    int unitPrice;     // 주문 당시 메뉴의 단가 (가격 변동 대비)
    nlohmann::json selectedOptions; // 주문 당시 스냅샷

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(OrderItemDTO, menuId, quantity, unitPrice, selectedOptions)
};

struct OrderCreateReqDTO {   // 주문 생성 요청 DTO
    std::string userId;      // 주문자 ID
    int storeId;             // 주문할 상점 ID
    int totalPrice;     // 주문 총액 (계산된 가격을 보내도록 클라이언트에게 명시)
    std::string deliveryAddress;    // 배달 주소 (고객이 입력한 대로)
    int couponId;       // 쿠폰 ID (쿠폰이 적용된 경우, 아니면 -1) 
    std::vector<OrderItemDTO> items;  // 주문 아이템 목록

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(OrderCreateReqDTO, userId, storeId, totalPrice, deliveryAddress, couponId, items)
};

struct OrderCreateResDTO {  // 주문 생성 응답 DTO
    int status;             // 0: 성공, 1: 실패 (예: 재고 부족, 결제 실패 등)   
    std::string message;   // 주문 생성 결과 메시지 (성공 시 "주문이 성공적으로 생성되었습니다.", 실패 시 구체적인 이유)    
    std::string orderId; // 발급된 주문번호

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(OrderCreateResDTO, status, message, orderId)
};