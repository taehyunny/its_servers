#pragma once
#include <string>
#include <vector>
#include "json.hpp"

struct CartItem
{
    int menuId;
    std::vector<int> optionIds; // 🚀 여기서 옵션 번호들을 std::sort() 해둘 겁니다!
    int quantity;
    int unitPrice;

    // 🚀 태현님의 아이디어 핵심: 두 장바구니 아이템이 "완벽히 같은지" 판별!
    bool operator==(const CartItem &other) const
    {
        // 메뉴 ID가 같고, 정렬된 옵션 리스트가 완벽히 같으면 동일 아이템!
        return (menuId == other.menuId) && (optionIds == other.optionIds);
    }
};
struct OrderItemDTO
{                                   // 주문 아이템 하나를 나타내는 DTO
    int menuId;                     // 주문한 메뉴의 ID
    int quantity;                   // 주문한 수량
    int unitPrice;                  // 주문 당시 메뉴의 단가 (가격 변동 대비)
    nlohmann::json selectedOptions; // 주문 당시 스냅샷

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(OrderItemDTO, menuId, quantity, unitPrice, selectedOptions)
};

struct OrderCreateReqDTO
{                                    // 주문 생성 요청 DTO
    std::string userId;              // 주문자 ID
    int storeId;                     // 주문할 상점 ID
    int totalPrice;                  // 주문 총액 (계산된 가격을 보내도록 클라이언트에게 명시)
    std::string deliveryAddress;     // 배달 주소 (고객이 입력한 대로)
    int couponId;                    // 쿠폰 ID (쿠폰이 적용된 경우, 아니면 -1)
    std::vector<OrderItemDTO> items; // 주문 아이템 목록

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(OrderCreateReqDTO, userId, storeId, totalPrice, deliveryAddress, couponId, items)
};

struct OrderCreateResDTO
{                        // 주문 생성 응답 DTO
    int status;          // 0: 성공, 1: 실패 (예: 재고 부족, 결제 실패 등)
    std::string message; // 주문 생성 결과 메시지 (성공 시 "주문이 성공적으로 생성되었습니다.", 실패 시 구체적인 이유)
    std::string orderId; // 발급된 주문번호

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(OrderCreateResDTO, status, message, orderId)
};

// 🧑‍🍳 1. 사장님의 주문 수락 요청 (REQ_ORDER_ACCEPT = 3000)
struct OrderAcceptReqDTO
{
    std::string orderId; // 수락할 주문번호 (예: "ORD-1710992345678")
    int estimatedTime;   // 예상 조리 시간 (분 단위, 예: 40)

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(OrderAcceptReqDTO, orderId, estimatedTime)
};

// 🧑‍🍳 2. 서버 -> 사장님 수락 결과 응답 (RES_ORDER_ACCEPT = 3001)
struct OrderAcceptResDTO
{
    int status;          // 0: 성공, 1: 실패 (DB 에러 등)
    std::string message; // "주문이 성공적으로 수락되었습니다."

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(OrderAcceptResDTO, status, message)
};

// 🙋‍♂️ 3. 서버 -> 고객 실시간 상태 푸시 (NOTIFY_ORDER_STATE = 9010)
struct NotifyOrderStateDTO
{
    std::string orderId; // 상태가 변경된 주문번호
    int state;           // 1: 조리중, 2: 조리완료, 3: 배달중 등 상태 코드
    std::string message; // 고객 폰에 띄울 팝업 메시지 ("사장님이 조리를 시작했습니다!")

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(NotifyOrderStateDTO, orderId, state, message)
};

// 🏍️ 4. 서버 -> 라이더들 실시간 콜 브로드캐스트 (NOTIFY_DELIVERY_CALL = 9020)
struct NotifyDeliveryCallDTO
{
    std::string orderId;         // 배달할 주문번호
    std::string pickupAddress;   // 픽업지 (황궁짜장 주소)
    std::string deliveryAddress; // 도착지 (고객 주소)
    int deliveryFee;             // 라이더가 받을 배달료 (예: 3500)

    // (참고: 나중에 메뉴 요약 정보 "짜장면 외 1건" 같은 걸 추가해도 좋습니다!)
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(NotifyDeliveryCallDTO, orderId, pickupAddress, deliveryAddress, deliveryFee)
};

// 🚀 [2026] 결제 화면 정보 요청 DTO
struct ReqCheckoutInfoDTO
{
    std::string userId;
    int storeId;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ReqCheckoutInfoDTO, userId, storeId)
};

// 🚀 [2027] 결제 화면 정보 응답 DTO
struct ResCheckoutInfoDTO
{
    int status;

    // --- 유저 정보 (CUSTOMERS 테이블) ---
    std::string customerGrade; // "일반" or "와우"
    std::string cardNumber;    // "1234-5678-****-****" (없으면 빈 문자열)
    std::string accountNumber; // "국민 123-456-789" (없으면 빈 문자열)
    int userPoint;             // (보너스) 보유 포인트도 보여주면 좋겠죠?

    // --- 매장 정보 (STORES 테이블) ---
    int minOrderAmount; // 최소주문금액
    int deliveryFee;    // 배달비 (와우회원이면 프론트에서 0원으로 처리 가능!)

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ResCheckoutInfoDTO,
                                   status, customerGrade, cardNumber, accountNumber, userPoint, minOrderAmount, deliveryFee)
};