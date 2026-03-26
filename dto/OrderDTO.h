#pragma once
#include <string>
#include <vector>
#include "json.hpp"

struct CartItem // 장바구니 아이템 구조체 (주문 생성 시 클라이언트 내부에서 사용)
{
    int menuId;                 // 주문한 메뉴의 ID
    std::vector<int> optionIds; // 🚀 여기서 옵션 번호들을 std::sort() 해둘 겁니다!
    int quantity;               // 주문 수량
    int unitPrice;              // 주문 당시 메뉴의 단가 (가격 변동 대비)

    // 🚀 태현님의 아이디어 핵심: 두 장바구니 아이템이 "완벽히 같은지" 판별!
    bool operator==(const CartItem &other) const
    {
        // 메뉴 ID가 같고, 정렬된 옵션 리스트가 완벽히 같으면 동일 아이템!
        return (menuId == other.menuId) && (optionIds == other.optionIds);
    }
};
struct OrderItemDTO // 주문 생성 시 클라이언트가 보내는 주문 아이템 DTO
{
    int menuId;                     // 주문한 메뉴의 ID
    int quantity;                   // 주문한 수량
    int unitPrice;                  // 주문 당시 메뉴의 단가 (가격 변동 대비)
    nlohmann::json selectedOptions; // 주문 당시 스냅샷
    std::string menuName;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(OrderItemDTO, menuId, quantity, unitPrice, selectedOptions, menuName)
};

struct OrderCreateReqDTO // 주문 생성 요청 DTO (클라이언트 -> 서버)
{
    std::string userId;              // 주문자 ID
    int storeId;                     // 주문할 상점 ID
    int totalPrice;                  // 주문 총액 (계산된 가격을 보내도록 클라이언트에게 명시)
    std::string deliveryAddress;     // 배달 주소 (고객이 입력한 대로)
    int couponId;                    // 쿠폰 ID (쿠폰이 적용된 경우, 아니면 -1)
    std::vector<OrderItemDTO> items; // 주문 아이템 목록
    std::string storeRequest;        // 매장 요청사항 (예: "매운맛으로 해주세요!")
    std::string riderRequest;        // 라이더 요청사항 (예: "문 앞에 놔주세요!")

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(OrderCreateReqDTO, userId, storeId, totalPrice, deliveryAddress, storeRequest, riderRequest, items)
};

struct OrderCreateResDTO
{                        // 주문 생성 응답 DTO
    int status;          // 0: 성공, 1: 실패 (예: 재고 부족, 결제 실패 등)
    std::string message; // 주문 생성 결과 메시지 (성공 시 "주문이 성공적으로 생성되었습니다.", 실패 시 구체적인 이유)
    std::string orderId; // 발급된 주문번호

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(OrderCreateResDTO, status, message, orderId)
};

// 🧑‍🍳 1. 사장님의 주문 수락 요청 (REQ_ORDER_ACCEPT = 3000)
struct OrderAcceptReqDTO // 사장님이 주문을 수락할 때 보내는 요청 DTO (예: REQ_ORDER_ACCEPT)
{
    std::string orderId; // 수락할 주문번호 (예: "ORD-1710992345678")
    int estimatedTime;   // 예상 조리 시간 (분 단위, 예: 40)

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(OrderAcceptReqDTO, orderId, estimatedTime)
};

// 🧑‍🍳 2. 서버 -> 사장님 수락 결과 응답 (RES_ORDER_ACCEPT = 3001)
struct OrderAcceptResDTO // 서버 -> 사장님: "이 주문 수락 결과입니다!" 응답 DTO
{
    int status;          // 0: 성공, 1: 실패 (DB 에러 등)
    std::string message; // "주문이 성공적으로 수락되었습니다."
    std::string orderId; // 수락된 주문번호 (클라이언트가 어떤 주문이 수락됐는지 알 수 있도록)
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(OrderAcceptResDTO, status, message, orderId)
};

// 🙋‍♂️ 3. 서버 -> 고객 실시간 상태 푸시 (NOTIFY_ORDER_STATE = 9010)
struct NotifyOrderStateDTO // 서버 -> 고객: "주문 상태가 변경되었습니다!" (푸시 알림용)
{
    std::string orderId; // 상태가 변경된 주문번호
    int state;           // 1: 조리중, 2: 조리완료, 3: 배달중 등 상태 코드
    std::string message; // 고객 폰에 띄울 팝업 메시지 ("사장님이 조리를 시작했습니다!")

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(NotifyOrderStateDTO, orderId, state, message)
};

// 🏍️ 4. 서버 -> 라이더들 실시간 콜 브로드캐스트 (NOTIFY_DELIVERY_CALL = 9020)
struct NotifyDeliveryCallDTO //    서버 -> 라이더: "새로운 배달 콜이 들어왔습니다!" (브로드캐스트용)
{
    std::string orderId;         // 배달할 주문번호
    std::string pickupAddress;   // 픽업지 (매장 주소)
    std::string deliveryAddress; // 도착지 (고객 주소)
    int deliveryFee;             // 라이더가 받을 배달료

    // 🚀 클라이언트 요청 추가 데이터
    std::string menuSummary; // "떡볶이 외 1건"
    int totalPrice;          // 총 결제 금액
    std::string createdAt;   // 주문 생성 시간        // 라이더가 받을 배달료 (예: 3500)

    // (참고: 나중에 메뉴 요약 정보 "짜장면 외 1건" 같은 걸 추가해도 좋습니다!)
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(NotifyDeliveryCallDTO, orderId, pickupAddress, deliveryAddress, deliveryFee, menuSummary, totalPrice, createdAt)
};

// 🚀 [2026] 결제 화면 정보 요청 DTO
struct ReqCheckoutInfoDTO // 클라이언트 -> 서버: "이 주문의 영수증 정보 다 주세요!" 요청 DTO
{
    std::string userId; // 주문자 ID
    int storeId;        // 주문할 상점 ID

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(ReqCheckoutInfoDTO, userId, storeId)
};

struct ResCheckoutInfoDTO // 서버 -> 클라이언트: "여기 주문 상세 영수증 정보입니다!" 응답 DTO
{
    int status; // 0: 성공, 1: 실패 (예: 사용자 정보 조회 실패, 매장 정보 조회 실패 등)

    // --- 유저 정보 (CUSTOMERS 테이블) ---
    std::string customerGrade; // "일반" or "와우"
    std::string cardNumber;    // "1234-5678-****-****"
    std::string accountNumber; // "국민 123-456-789"
    int userPoint;             // 보유 포인트
    std::string userAddress;   // 🚀 고객 주소 (배달용)

    // --- 매장 정보 (STORES 테이블) ---
    int minOrderAmount; // 최소주문금액
    int deliveryFee;    // 배달비 (포장일 땐 0원, 와우일 땐 0원 처리)

    // 🚀 [클라이언트 요청 사항] 포장용 필드 추가
    std::string storeAddress; // 매장의 실제 주소
    std::string pickupTime;   // 예: "15~25분 후 방문 포장"

    // (보너스) 총 주문 금액도 내려주면 프론트가 편해집니다
    int totalPrice; // 계산된 총 주문 금액 (상품 가격 + 옵션 가격 - 쿠폰 할인 + 배달비)

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(ResCheckoutInfoDTO,
                                                status, customerGrade, cardNumber, accountNumber, userPoint, userAddress,
                                                minOrderAmount, deliveryFee, storeAddress, pickupTime, totalPrice)
};
// 🧑‍🍳 [추가 제안] 만능 주문 상태 변경 요청 (예: REQ_CHANGE_ORDER_STATE = 3020)
struct ReqChangeOrderStateDTO // 사장님이 주문 상태를 변경할 때 보내는 요청 DTO
{
    std::string orderId; // 상태를 바꿀 주문번호
    int newState;        // 변경할 상태값 (예: 2=조리완료, 3=배달출발, 4=배달완료)

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(ReqChangeOrderStateDTO, orderId, newState)
};

// 🧑‍🍳 [추가 제안] 만능 상태 변경 응답
struct ResChangeOrderStateDTO // 서버 -> 클라이언트: "주문 상태 변경 결과입니다!" 응답 DTO
{
    int status;          // 0: 성공, 1: 실패
    std::string message; /// "주문 상태가 성공적으로 변경되었습니다." 또는 실패 이유 메시지

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(ResChangeOrderStateDTO, status, message)
};
struct NotifyNewOrderDTO // 서버 -> 사장님: "새로운 주문이 들어왔습니다!" (푸시 알림용)
{
    std::string orderId;             // 주문 번호
    std::string userId;              // 주문자 ID
    std::string menuSummary;         // "떡볶이 외 1건"
    int totalPrice;                  // 총 결제 금액
    std::string deliveryAddress;     // 배달 주소
    std::string createdAt;           // 주문 생성 시간
    std::vector<OrderItemDTO> items; // 주문 아이템 상세 정보 (메뉴 ID, 수량, 옵션 등)
    std::string storeRequest;        // 🚀 사장님 요청사항 추가
    std::string riderRequest;        // 🚀 라이더 요청사항 추가

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(NotifyNewOrderDTO,
                                                orderId, userId, menuSummary, totalPrice,
                                                deliveryAddress, createdAt, items, storeRequest, riderRequest)
};

// 🧑‍🍳 주문 거절 요청 (REQ_ORDER_REJECT = 3010)
struct ReqOrderRejectDTO // 사장님이 주문을 거절할 때 보내는 요청 DTO
{
    std::string orderId; // 거절할 주문 번호
    std::string reason;  // 🚀 핵심: 거절 사유 ("재료 소진", "영업 종료" 등)

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ReqOrderRejectDTO, orderId, reason)
};

// 🧑‍🍳 주문 거절 응답 (RES_ORDER_REJECT = 3011)
struct ResOrderRejectDTO
{
    int status;          // 0: 성공, 1: 실패
    std::string message; // "주문 거절 처리가 완료되었습니다."
    std::string orderId; // 거절된 주문 번호 (클라이언트가 어떤 주문이 거절됐는지 알 수 있도록)
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ResOrderRejectDTO, status, message, orderId)
};

// 🙋‍♂️ 주문 내역 하나를 나타내는 단위
struct OrderHistoryItemDTO // 주문 내역 리스트에서 각 주문을 나타내는 DTO
{
    std::string orderId;          // 주문 번호
    int storeId;                  // 매장 ID (주문 내역 리스트에서 매장 이름과 매칭하기 위해 필요)
    std::string storeName;        // 매장 이름 (주문 내역 리스트에서 바로 보여줄 수 있도록)
    std::string menuName;         // 대표 메뉴 이름 (예: "짜장면" or "떡볶이")
    int totalPrice;               // 총 결제 금액
    int status;                   // 주문 상태 (예: 1=조리중, 2=조리완료, 3=배달중, 4=배달완료)
    std::string menuSummary;      // "짜장면 외 1건"
    std::string createdAt;        // 주문 생성 일시 (예: "2024-08-15 12:34:56")
    std::string deliveryPhotoUrl; // 배달 완료 사진 (영수증 대용)

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(OrderHistoryItemDTO,
                                   orderId, storeId, storeName, menuName, totalPrice, status, menuSummary, createdAt, deliveryPhotoUrl)
};

// 🙋‍♂️ 2081: RES_ORDER_HISTORY
struct ResOrderHistoryDTO // 서버 -> 클라이언트: "여기 주문 내역 리스트입니다!" 응답 DTO
{
    int status;                                   // 0: 성공, 1: 실패
    std::vector<OrderHistoryItemDTO> historyList; // 주문 내역 리스트 (각 주문은 OrderHistoryItemDTO로 표현)

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ResOrderHistoryDTO, status, historyList)
};

// 🙋‍♂️ 2080: 일반 과거 주문 내역 요청
struct ReqOrderHistoryDTO // 클라이언트 -> 서버: "내 과거 주문 내역 다 주세요!" 요청 DTO
{
    std::string userId;                                        // 주문자 ID (주문 내역을 조회할 유저의 ID)
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ReqOrderHistoryDTO, userId) // JSON <-> Struct 자동 변환 매크로
};

// 🙋‍♂️ 2082: 검색어가 포함된 주문 내역 요청
struct ReqOrderHistorySearchDTO // 클라이언트 -> 서버: "내 주문 내역 중에서 이 검색어가 포함된 것만 주세요!" 요청 DTO
{
    std::string userId;  // 주문자 ID (주문 내역을 조회할 유저의 ID)
    std::string keyword; // 검색어 (매장명 or 메뉴명)
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(ReqOrderHistorySearchDTO, userId, keyword)
};

struct ReqOrderDetailDTO
{                        // 클라이언트 -> 서버: "이 주문의 상세 영수증 정보 다 주세요!" 요청 DTO
    std::string orderId; // 어떤 주문의 영수증인지

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ReqOrderDetailDTO, orderId)
};

