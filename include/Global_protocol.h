#pragma once // 중복 포함 방지 필수!

#include <string>
#include <cstdint>
#include "json.hpp"

using json = nlohmann::json;

// =========================================================================
// 1. 명령어 ID (PacketHeader에서 사용하기 위해 먼저 선언)
// 파트별로 번호 대역을 분리하여 각 클라이언트의 책임(R&R)을 명확히 함
// =========================================================================
enum class CmdID : uint16_t
{
    // ---------------------------------------------------------
    // [1000번대] 공통 및 시스템 (인증, 연결 유지)
    // ---------------------------------------------------------
    REQ_HEARTBEAT = 1000, // 클라이언트 -> 서버: 실시간 연결 유지 확인
    RES_HEARTBEAT = 1001, // 서버 -> 클라이언트: 연결 유지 응답

    REQ_LOGIN = 1010,          // 공통: 로그인 요청 (고객, 사장님, 라이더 모두 사용)
    RES_LOGIN = 1011,          // 공통: 로그인 응답
    REQ_SIGNUP = 1020,         // 공통: 통합 회원가입 요청
    RES_SIGNUP = 1021,         // 공통: 통합 회원가입 응답
    REQ_LOGOUT = 1030,         // 공통: 로그아웃 요청
    RES_LOGOUT = 1031,         // 공통: 로그아웃 응답
    REQ_AUTH_CHECK = 1040,     // 공통: 아이디 중복 확인 요청
    RES_AUTH_CHECK = 1041,     // 공통: 아이디 중복 확인 응답
    REQ_PHONE_CHECK = 1042,    // 공통: 전화번호 중복 확인 요청
    RES_PHONE_CHECK = 1043,    // 공통: 전화번호 중복 확인 응답
    REQ_PROFILE_UPDATE = 1050, // 공통: 프로필 업데이트 요청
    RES_PROFILE_UPDATE = 1051, // 공통: 프로필 업데이트 응답

    REQ_CATEGORY = 1060, // 공통: 카테고리 목록 요청 (최초 1회 후 클라이언트 캐싱)
    RES_CATEGORY = 1061, // 공통: 카테고리 목록 응답

    REQ_WITHDRAW = 1070, // 공통: 회원탈퇴 요청
    RES_WITHDRAW = 1071, // 공통: 회원탈퇴 응답

    // ---------------------------------------------------------
    // [2000번대] 고객(Customer) 파트
    //
    // [매장/메뉴 조회]   2000 ~ 2019
    // [주문/결제]        2020 ~ 2039
    // [리뷰]             2030 ~ 2039  ← REVIEW_WRITE(2030) 포함
    // [즐겨찾기]         2040 ~ 2049
    // [쿠폰]             2050 ~ 2059
    // [실시간 주문현황]   2060 ~ 2069
    // [주소 관리]        2070 ~ 2079
    // [주문 내역]        2080 ~ 2099
    // [채팅 상담]        2090 ~ 2095  ← 2080 이후 여유 공간 활용
    // [마이페이지]       2100 ~ 2109
    // ---------------------------------------------------------

    REQ_BUISNESS_NUM_CHECK = 2006, // 사업자번호 중복 확인 요청 (사장님 회원가입 시)
    RES_BUISNESS_NUM_CHECK = 2007, // 사업자번호 중복 확인 응답
                                   // 매장 / 메뉴 조회 (2000 ~ 2019)
    REQ_STORE_LIST = 2000,         // 배달 가능 반경 내 상점 목록 요청 (정렬·필터 파라미터 포함)
    RES_STORE_LIST = 2001,         // 상점 목록 응답 (최소주문금액, 배달예상시간, 별점 등 포함)
    REQ_STORE_DETAIL = 2002,       // 특정 매장 상세 정보 요청 (상호명, 주소, 영업시간, 포장 가능 여부 등)
    RES_STORE_DETAIL = 2003,       // 매장 상세 정보 응답

    REQ_MENU_LIST = 2010,   // 특정 상점 메뉴 목록 요청 (카테고리별, 품절 여부 포함)
    RES_MENU_LIST = 2011,   // 메뉴 목록 응답
    REQ_MENU_OPTION = 2012, // 특정 메뉴의 추가 선택 옵션 요청 (필수/선택 옵션)
    RES_MENU_OPTION = 2013, // 메뉴 옵션 응답
    REQ_REVIEW_LIST = 2014, // 매장 리뷰 목록 요청 (필터·정렬 파라미터 포함)
    RES_REVIEW_LIST = 2015, // 리뷰 목록 응답 (주문 메뉴 버튼 포함)
    REQ_MENU_REVIEW_LIST = 2016, // 특정 메뉴 리뷰 목록 요청 (필터·정렬 파라미터 포함)
    RES_MENU_REVIEW_LIST = 2017, // 특정 메뉴 리뷰 목록 응답
    // 주문 / 결제 (2020 ~ 2029)
    REQ_ORDER_CREATE = 2020,    // 장바구니 결제 및 주문 생성 요청
    RES_ORDER_CREATE = 2021,    // 주문 생성 응답
                                // REQ_COUPON_CHECK = 2022, // 쿠폰 유효성 확인 요청
                                // RES_COUPON_CHECK = 2023, // 쿠폰 유효성 확인 응답
    REQ_PAYMENT_PROCESS = 2024, // 클라이언트 -> 서버: 결제 모듈(PG) 승인 후 결과 전달
    RES_PAYMENT_PROCESS = 2025, // 서버 -> 클라이언트: 결제 완료 및 영수증 ID 반환
    REQ_CHECKOUT_INFO = 2026,   // 클라이언트 -> 서버: 결제 직전 최종 주문 정보 요청 (최종 금액, 예상 배달 시간 등)
    RES_CHECKOUT_INFO = 2027,   // 서버 -> 클라이언트: 결제 직전 최종 주문 정보 응답 (최종 금액, 예상 배달 시간 등)
                                // 리뷰 작성 (2030 ~ 2035)
    REQ_REVIEW_WRITE = 2030,    // 사진 첨부 리뷰 작성 요청 (음식별점, 배달별점, 좋아요/싫어요)
    RES_REVIEW_WRITE = 2031,    // 리뷰 작성 응답

    // 즐겨찾기 (2040 ~ 2049)
    REQ_FAVORITE_ADD = 2040,    // 즐겨찾기 추가 요청
    RES_FAVORITE_ADD = 2041,    // 즐겨찾기 추가 응답
    REQ_FAVORITE_DELETE = 2042, // 즐겨찾기 삭제 요청
    RES_FAVORITE_DELETE = 2043, // 즐겨찾기 삭제 응답
    REQ_FAVORITE_LIST = 2044,   // 즐겨찾기 목록 요청 (자주/최근주문/최근추가 정렬 파라미터)
    RES_FAVORITE_LIST = 2045,   // 즐겨찾기 목록 응답

    // 실시간 주문 현황 (2050 ~ 2059)
    REQ_ORDER_LIST = 2050,   // 실시간 주문 확인 요청 (배달/포장 상태, 타이머 포함)
    RES_ORDER_LIST = 2051,   // 실시간 주문 확인 응답
    REQ_ORDER_CANCEL = 2052, // 주문 취소 요청 (가게 수락 전에만 가능)
    RES_ORDER_CANCEL = 2053, // 주문 취소 응답

    // 주소 관리 (2070 ~ 2079)  ← 기존 번호 유지
    REQ_ADDRESS_SAVE = 2070,
    RES_ADDRESS_SAVE = 2071,
    REQ_ADDRESS_LIST = 2072,
    RES_ADDRESS_LIST = 2073,
    REQ_ADDRESS_DELETE = 2074,
    RES_ADDRESS_DELETE = 2075,
    REQ_ADDRESS_UPDATE = 2076,
    RES_ADDRESS_UPDATE = 2077,
    REQ_ADDRESS_DEFAULT = 2078,
    RES_ADDRESS_DEFAULT = 2079,

    // 과거 주문 내역 (2080 ~ 2089)
    REQ_ORDER_HISTORY = 2080,        // 과거 주문 내역 목록 요청 (최근 주문 순 추천 포함)
    RES_ORDER_HISTORY = 2081,        // 과거 주문 내역 목록 응답 (배달완료사진, 영수증 등)
    REQ_ORDER_HISTORY_SEARCH = 2082, // 과거 주문 내역 메뉴·매장명 검색 요청
    RES_ORDER_HISTORY_SEARCH = 2083, // 과거 주문 내역 검색 응답
    REQ_ORDER_REORDER = 2084,        // 재주문 요청 (동일 메뉴 장바구니 복원)
    RES_ORDER_REORDER = 2085,        // 재주문 응답
    REQ_ORDER_DETAIL = 2086,         // 영수증·주문 상세 요청
    RES_ORDER_DETAIL = 2087,         // 영수증·주문 상세 응답
    REQ_ORDER_DELETE = 2088,         // 주문 내역 삭제 요청 (서버 동시 삭제)
    RES_ORDER_DELETE = 2089,         // 주문 내역 삭제 응답

    // 채팅 상담 (2090 ~ 2095)
    REQ_CHAT_CONNECT = 2090, // 관리자 1:1 채팅방 입장 요청
    RES_CHAT_CONNECT = 2091, // 채팅방 입장 응답
    REQ_CHAT_SEND = 2092,    // 메시지 전송 요청 (WebSocket 방식 검토 필요)
    RES_CHAT_SEND = 2093,    // 메시지 전송 응답
    REQ_GRADE_UPDATE = 2094, // 등급 변경 요청
    RES_GRADE_UPDATE = 2095, // 등급 변경 응답 (업그레이드/다운그레이드 구분 없이 같은 CmdID로 처리)
    REQ_GRADE_NOW = 2096,    // 현재 등급 요청
    RES_GRADE_NOW = 2097,    // 현재 등급 응답
                             // 마이페이지 (2100 ~ 2109)
    REQ_MY_INFO = 2100,      // 마이페이지 통합 정보 요청 (리뷰수, 주문수, 즐겨찾기수, 좋아요수)
    RES_MY_INFO = 2101,      // 마이페이지 통합 정보 응답

    REQ_STORE_INFO_UPDATE = 2102,  // 매장 정보 업데이트 요청 (영업시간, 휴무일 등)
    RES_STORE_INFO_UPDATE = 2103,  // 매장 정보 업데이트 응답
    REQ_RESEARCH_WIDGET = 2108,    // 매장 검색 위젯 요청 (최근 검색어 출력용)
    RES_RESEARCH_WIDGET = 2109,    // 매장 검색 위젯 응답 (최근 검색어 리스트)
    REQ_RESEARCH_DELETE = 2110,    // 매장 검색 위젯 개별 삭제 요청 (최근 검색어 삭제)
    RES_RESEARCH_DELETE = 2111,    // 매장 검색 위젯 개별 삭제 응답
    REQ_RESEARCH_ADD = 2112,       // 매장 검색 위젯 추가 요청 (최근 검색어 추가)
    RES_RESEARCH_ADD = 2113,       // 매장 검색 위젯 추가 응답
    REQ_RESEARCH_DEL_ALL = 2114,   // 매장 검색 위젯 전체 삭제 요청 (최근 검색어 전체 삭제)
    RES_RESEARCH_DEL_ALL = 2115,   // 매장 검색 위젯 전체 삭제 응답
    REQ_SEARCH_STORE = 2116,       // 매장 검색 요청 (검색어로 매장명·카테고리 검색)
    RES_SEARCH_STORE = 2117,       // 매장 검색 응답
                                   // ---------------------------------------------------------
                                   // [3000번대] 사장님(Owner) 파트
                                   // ---------------------------------------------------------
    REQ_ORDER_ACCEPT = 3000,       // 주문 수락 및 조리 시작 요청 (예상 시간 포함)
    RES_ORDER_ACCEPT = 3001,       // 주문 수락 응답
    REQ_ORDER_REJECT = 3010,       // 주문 거절 요청 (품절 등 사유 포함)
    RES_ORDER_REJECT = 3011,       // 주문 거절 응답
    REQ_COOK_TIME_SET = 3020,      // 조리 시간 재설정 요청
    RES_COOK_TIME_SET = 3021,      // 조리 시간 설정 응답
    REQ_CHANGE_ORDER_STATE = 3022, // 주문 상태 변경 요청 (조리 시작, 조리 완료, 배달 출발 등)
    RES_CHANGE_ORDER_STATE = 3023, // 주문 상태 변경 응답 (조리 시작, 조리 완료, 배달 출발 등)

    REQ_STORE_STATUS_SET = 3030,    // 영업 상태 수정 요청
    RES_STORE_STATUS_SET = 3031,    // 영업 상태 응답
    REQ_DELIVERY_POLICY_SET = 3040, // 배달 반경 및 배달료 수정 요청
    RES_DELIVERY_POLICY_SET = 3041, // 배달 반경 및 배달료 수정 응답
    REQ_MENU_SOLD_OUT = 3050,       // 메뉴 품절 처리
    RES_MENU_SOLD_OUT = 3051,       // 메뉴 품절 응답
    REQ_MENU_EDIT = 3060,           // 메뉴 정보 편집 요청
    RES_MENU_EDIT = 3061,           // 메뉴 정보 편집 응답
    REQ_REVIEW_REPLY = 3070,        // 리뷰 답글 작성 요청
    RES_REVIEW_REPLY = 3071,        // 리뷰 답글 작성 응답
    REQ_SALES_STAT = 3090,          // 매출 및 정산 대시보드 요청
    RES_SALES_STAT = 3091,          // 매출 및 정산 대시보드 응답
    REQ_SETTLEMENT_INFO = 3100,     // 정산 내역 조회 요청
    RES_SETTLEMENT_INFO = 3101,     // 정산 내역 조회 응답
    REQ_COUPON_MANAGE = 3110,       // 쿠폰 발급 요청
    RES_COUPON_MANAGE = 3111,       // 쿠폰 발급 응답
    REQ_BLACKLIST_REQUEST = 3120,   // 차단 요청
    RES_BLACKLIST_REQUEST = 3121,   // 차단 응답

    // ---------------------------------------------------------
    // [4000번대] 라이더(Rider) 파트
    // ---------------------------------------------------------
    REQ_DELIVERY_ACCEPT = 4000,   // 서버가 뿌린 배달 콜 수락 요청
    RES_DELIVERY_ACCEPT = 4001,   // 배달 콜 수락 응답
    REQ_PICKUP = 4002,            // 픽업 완료 요청
    RES_PICKUP = 4003,            // 픽업 완료 응답
    REQ_DELIVERY_COMPLETE = 4010, // 배달 완료 요청
    RES_DELIVERY_COMPLETE = 4011, // 배달 완료 응답
                                  // 🚀 라이더용 신규 추가
    REQ_RIDER_ORDER_LIST = 4020,  // 라이더 배달 가능 주문 목록 요청
    RES_RIDER_ORDER_LIST = 4021,  // 라이더 배달 가능 주문 목록 응답
                                  // ---------------------------------------------------------
                                  // [5000번대] 관리자(Admin) 파트
                                  // ---------------------------------------------------------
    RES_REQUEST_OK = 5000,        // 1대1 문의 요청 동의
    RES_REQUEST_NO = 5001,        // 1대1 문의 요청 비동의

    REQ_REFUND = 5010, // 환불 요청
    REQ_CANCEL = 5011, // 취소 요청
    RES_REFUND = 5012, // 환불 요청 응답
    RES_CANCEL = 5013, // 취소 요청 응답

    REQ_ADMIN_ORDER_LIST = 5020, // 주문 내역 검색 (관리자용, 고객 2050과 별도)
    RES_ADMIN_ORDER_LIST = 5021, // 주문 내역 응답

    REQ_RIDER_COUNT = 5030, // 출근한 라이더 수 요청
    RES_RIDER_COUNT = 5031, // 출근한 라이더 수 응답

    REQ_UNABLE_DELIVERY = 5040, // 배달 불가 상태 알림 (서버 응답 : 브로드캐스팅)
    REQ_SURCHARGE = 5041,       // 할증 요청             (서버 응답 : 브로드캐스팅)

    RES_REVIEW_DELETE_OK = 5050, // 악성 리뷰 삭제 동의
    RES_REVIEW_DELETE_NO = 5051, // 악성 리뷰 삭제 비동의

    REQ_ADMIN_INIT = 5090,       // 클라이언트 -> 서버: UI 없는 관리자 백그라운드 인증 요청
    RES_ADMIN_INIT = 5091,       // 서버 -> 클라이언트: 인증 완료 응답
                                 // ---------------------------------------------------------
                                 // [9000번대] 서버 푸시 알림 (Server Broadcast)
                                 // ---------------------------------------------------------
    NOTIFY_NEW_ORDER = 9000,     // 서버 -> 사장님: 새 주문 발생 알림
    NOTIFY_ORDER_STATE = 9010,   // 서버 -> 고객: 조리 시작, 배달 출발 등 상태 변경 알림
    NOTIFY_DELIVERY_CALL = 9020, // 서버 -> 라이더들: 주변 매장의 새 배달 콜 알림
    NOTIFY_CHAT_MSG = 9030,      // 서버 -> 고객: 관리자 채팅 메시지 수신 알림
    NOTIFY_ADMIN_CHAT_REQ = 9040,
};

// =========================================================================
// 2. 패킷 헤더 (1바이트 정렬 강제)
// =========================================================================
#pragma pack(push, 1)
struct PacketHeader
{                                // 모든 패킷의 맨 앞에 위치하여 데이터의 이정표 역할을 함
    uint16_t signature = 0x4543; // 기본값 할당 ('E','C' - Eats Connect)
    CmdID cmdId;                 // 명령어 ID (어떤 파트의 어떤 요청인지 식별)
    uint32_t bodySize;           // 뒤따라오는 JSON 바디의 실제 크기 (바이트 단위)
};
#pragma pack(pop) // [주의] DTO 선언 전에 반드시 pack 설정을 해제해야 함!

enum class SignupResult
{
    SUCCESS = 1,
    DUPLICATE_ID = -1,
    DUPLICATE_PHONE = -2, // 태현님 DTO에 맞춰서 전화번호 중복으로 예시를 들게요! (이메일이면 PHONE 대신 EMAIL 사용)
    SERVER_ERROR = -99
};

enum class LoginResult
{
    SUCCESS = 1,
    ID_PASS_WRONG = -1,
    SERVER_ERROR = -99
};