# Its_bedalyo Server Prototype

배달 주문 흐름에서 주문 당시 메뉴·가격·결제 정보를 서버 기준 스냅샷으로 저장하는 팀 프로젝트 프로토타입입니다.

## Portfolio Evidence

- Role: Team lead / server flow owner
- Scope: C++ TCP/IP protocol, 8-byte header, JSON body, MariaDB order snapshot flow
- Evidence: protocol rule, DTO structure, server request flow, snapshot schema
- Boundary: This is a team prototype, not a production service.

## What this project proves

이 프로젝트는 주문 이후 메뉴·가격이 바뀌어도 결제 당시 기준의 주문 데이터가 유지되도록, 서버 요청과 DB 저장 기준을 분리해 설계한 흐름을 보여줍니다.

## Architecture Summary

```text
Client Order UI
-> TCP/IP Request
-> 8-byte Header + JSON Body
-> Server-side validation
-> MariaDB order snapshot
-> Review / order history check
```

## Protocol Summary

| Field | Size | Purpose |
|---|---:|---|
| Signature | 2 bytes | 프로젝트 패킷 식별 |
| CmdID | 2 bytes | 요청 기능 구분 |
| BodySize | 4 bytes | JSON Body 크기 |
| JSON Body | variable | 실제 요청 데이터 |

## Data Snapshot Fields

| Field | Purpose |
|---|---|
| order_id | 주문 식별자 |
| menu_snapshot | 주문 당시 메뉴 정보 |
| price_snapshot | 주문 당시 가격 정보 |
| payment_state | 결제 상태 |
| created_at | 주문 생성 시각 |

## Environment Example

실제 접속 정보는 공개 저장소에 포함하지 않습니다. 로컬 실행 시 `.env.example`을 참고해 환경 변수를 설정합니다.

```env
DB_HOST=your-db-host
DB_PORT=3306
DB_NAME=your-db-name
DB_USER=your-db-user
DB_PASSWORD=your-password
SERVER_PORT=8080
```

## Server Evidence Map

| Evidence | Where to look | What it shows |
|---|---|---|
| Protocol rule | `include/Global_protocol.h`, packet framing code | 8-byte header와 JSON body 기준 |
| DTO structure | `dto/`, `server/src/handler/` | 요청·응답 데이터 구조 |
| Server request flow | `server/src/network/`, `server/src/service/` | TCP 요청 수신과 서버 검증 흐름 |
| Snapshot schema | `server/src/database/`, DAO layer | 주문 당시 데이터 저장 기준 |

## Review Focus

포트폴리오 검토 시에는 기능 목록보다 아래 흐름을 중심으로 확인해 주세요.

1. 주문 요청이 TCP/IP 패킷으로 전달되는 방식
2. 8-byte header와 JSON body로 요청을 구분하는 방식
3. 서버에서 주문 요청을 검증하고 DTO/DAO 흐름으로 전달하는 방식
4. 주문 당시 메뉴·가격·결제 정보를 스냅샷 형태로 저장하는 기준
5. 주문 내역과 리뷰 화면에서 동일 기준의 주문 데이터를 확인하는 흐름

## Security Boundary

- 실제 서버 IP, DB 계정, 비밀번호는 README에 기록하지 않습니다.
- 공개 저장소에는 실행 구조와 검증 근거만 남깁니다.
- 운영 서비스가 아니라 팀 프로젝트 프로토타입입니다.
