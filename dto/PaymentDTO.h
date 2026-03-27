#pragma once
#include <string>
#include <vector>
#include "json.hpp"

using json = nlohmann::json;

// 🚀 [2024] 결제 처리 요청 DTO
struct ReqPaymentProcessDTO // 클라이언트 -> 서버: "결제 처리 해주세요!" 요청 DTO
{
    std::string orderId;   // 주문 번호 [cite: 8]
    std::string userId;    // 결제자 ID
    std::string payMethod; // 결제 수단 (CARD, KAKAO, TOSS 등)
    int totalAmount;       // 주문 총 금액 [cite: 8]
    int discountAmount;    // 쿠폰/포인트 할인 금액 [cite: 4]
    int actualPayAmount;   // 실제 결제 금액 (total - discount)

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ReqPaymentProcessDTO,
                                   orderId, userId, payMethod, totalAmount, discountAmount, actualPayAmount)
};

// 🚀 [2025] 결제 결과 및 영수증 정보 응답 DTO
struct ResPaymentProcessDTO // 서버 -> 클라이언트: "결제 처리 결과입니다!" 응답 DTO
{
    int status;             // 200: 성공, 400: 잔액부족 등
    std::string message;    // 결과 메시지
    long long paymentId;    // 생성된 영수증 고유 번호
    std::string approvedAt; // 결제 승인 시각 (YYYY-MM-DD HH:MM:SS)
    std::string receiptUrl; // (선택) 외부 매출전표 링크

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ResPaymentProcessDTO,
                                   status, message, paymentId, approvedAt, receiptUrl)
};

// [2087] 주문 상세 응답 (서버 -> 클라이언트) - 영수증용
// [2087] 주문 상세 응답 (서버 -> 클라이언트) - 영수증용
struct ResOrderDetailDTO
{
    int status;                  // 200: 성공, 400: 오류 등
    std::string orderId;         // 주문 번호
    std::string storeName;       // 매장 이름 (투썸플레이스 가재울뉴타운점)
    std::string createdAt;       // 주문 일시 (2023-06-22 HH:MM:SS)
    std::string paymentMethod;   // 결제 수단 (신한카드 결제 등)
    std::string deliveryAddress; // 배달 주소

    // 🚀 영수증 금액 상세 내역
    int totalMenuPrice; // 주문금액 (순수 메뉴 합계)
    int deliveryFee;    // 배달비
    int couponDiscount; // 🚀 추가: 일반 할인금액
    int wowDiscount;    // 🚀 추가: 와우회원 할인 금액
    int totalPrice;     // 총 결제금액 (totalMenuPrice + deliveryFee - 할인가)

    // 상세 메뉴 리스트 (메뉴명, 가격, 수량, 그리고 "옵션 문자열 배열" 포함)
    std::vector<OrderItemDTO> items;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ResOrderDetailDTO, status, orderId, storeName, createdAt,
                                   paymentMethod, deliveryAddress, totalMenuPrice,
                                   deliveryFee, couponDiscount, wowDiscount, totalPrice, items)
};