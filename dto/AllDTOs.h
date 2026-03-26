// dto/AllDTOs.h (퍼사드 파일)
#pragma once

// include "dto/각종DTO.h" 파일들을 한 곳에서 모아서 관리하는 퍼사드 헤더 파일
// 클라이언트 팀원들은 이 파일 하나만 #include 하면 모든 통신 준비 끝!
#include "BaseDTO.h"
#include "Global_protocol.h"
#include "AccountDTO.h"
#include "StoreDTO.h"
#include "OrderDTO.h"
#include "ResearchDTO.h"
#include "StoreDetailDTO.h"
#include "AddressDTO.h"
#include "PaymentDTO.h"
#include "ChatDTO.h"
#include "AdminDTO.h" // 🚀 관리자 전용 DTO도 여기에 포함!
#include "RiderDTO.h" // 🚀 라이더 전용 DTO도 여기에 포함!