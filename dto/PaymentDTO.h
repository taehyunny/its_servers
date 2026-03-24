#pragma once
#include <string>
#include <vector>
#include "json.hpp"

using json = nlohmann::json;

// 🚀 [2024] 결제 처리 요청 DTO
struct ReqPaymentProcessDTO
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
struct ResPaymentProcessDTO
{
    int status;             // 200: 성공, 400: 잔액부족 등
    std::string message;    // 결과 메시지
    long long paymentId;    // 생성된 영수증 고유 번호
    std::string approvedAt; // 결제 승인 시각 (YYYY-MM-DD HH:MM:SS)
    std::string receiptUrl; // (선택) 외부 매출전표 링크

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ResPaymentProcessDTO,
                                   status, message, paymentId, approvedAt, receiptUrl)
};