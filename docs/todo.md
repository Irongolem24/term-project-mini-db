# TODO List

## 마일스톤

| 회차 | 목표 | 상태 |
|------|------|------|
|   ~ 7 | REPL + 파서 — 명령어 입력/분기 동작 | 완료 |
| 7 ~ 8 | DB 구조체 + 연산 — 메모리에 데이터 저장/조회 | 완료 |
| 8 ~ 9 | 파서 ↔ DB 연결 — 명령어 실행 시 실제 데이터 변경 | 완료 |
|   ~ 9 | 파일 저장/로드 — 데이터 영속성 | 완료 |
|   ~ 9 | SELECT 고급 기능 — DISTINCT / ORDER BY / LIMIT | 완료 |
|   ~ 9 | 마무리 — 메모리 누수 검증, 테스트 | 진행 중 |
|   선택 | 해시테이블 — PK O(1) 조회 | 미시작 |

---

## 진행 현황

| 파일 | 상태 | 설명 |
|------|------|------|
| `main.c` | 완료 | REPL 루프, 자동 save/load, _CrtDumpMemoryLeaks |
| `parser.h / parser.c` | 완료 | tokenize, dispatch, parse_* 전체 + ORDER BY / LIMIT |
| `db.h` | 완료 | 구조체 정의 + WhereClause, Operator, Condition |
| `db.c` | 완료 | DB 연산 함수 전체 구현 |
| `storage.h / storage.c` | 완료 | 자동 save / load 파일 I/O |
| `hashtable.h / hashtable.c` | 선택 | 해시테이블 구현 (선택 사항) |

---

### 1. db.c
- [x] `db_create` — Database malloc + 초기화
- [x] `table_find` — 이름으로 테이블 탐색
- [x] `table_create` — 테이블 생성 + db linked list에 연결
- [x] `table_drop` — 테이블 + 모든 Row 메모리 해제
- [x] `row_insert` — Row malloc + cells 복사 + TEXT strdup
- [x] `row_find_by_pk` — PK로 Row 찾기
- [x] `row_matches_where` — WHERE 조건 평가 (AND/OR, =, !=, <, >, <=, >=)
- [x] `rows_update_where` — WHERE 조건에 맞는 Row 컬럼 값 교체
- [x] `rows_delete_where` — WHERE 조건에 맞는 Row 삭제
- [x] `cell_compare` — 두 Cell 비교 (INT / FLOAT / TEXT 모두 지원)
- [x] `db_free` — 전체 메모리 해제

### 2. parser ↔ db 연결
- [x] `parse_create` → `table_create` 호출
- [x] `parse_drop` → `table_drop` 호출
- [x] `parse_insert` → `row_insert` 호출 (값 파싱 + 타입 변환 + PK 중복 체크)
- [x] `parse_select` → 전체 / 특정 컬럼 / DISTINCT / WHERE / ORDER BY / LIMIT 지원
- [x] `parse_update` → `rows_update_where` 호출
- [x] `parse_delete` → `rows_delete_where` 호출
- [x] `parse_show` → 테이블 목록 출력
- [x] `parse_describe` → 컬럼 목록 출력

### 3. storage.c
- [x] `db_save` — 전체 DB를 텍스트로 저장
- [x] `db_load` — 파일 읽어서 DB 복원
- [x] 프로그램 시작 시 자동 로드, 종료 시 자동 저장

### 4. 마무리
- [x] `main.c` 종료 시 `db_free()` 호출
- [x] `_CrtDumpMemoryLeaks()` 추가 (메모리 누수 확인)
- [ ] `HELP` 명령어 출력 내용 완성
- [ ] TC-01 ~ TC-05 테스트 시나리오 수동 실행

### 5. 선택 — 해시테이블
- [ ] `ht_create(int bucket_count)` — 버킷 배열 malloc
- [ ] `ht_insert(HashTable*, Cell* pk, Row*)` — 해시 계산 후 버킷에 연결
- [ ] `ht_find(HashTable*, Cell* pk)` — O(1) 조회
- [ ] `ht_delete(HashTable*, Cell* pk)` — 버킷에서 제거
- [ ] `ht_free(HashTable*)` — 버킷 배열 해제
- [ ] `row_insert` / `row_delete`에 해시테이블 연동

---

## 스코프 및 한계

### WHERE 문
- 최대 4개 조건 (`MAX_CONDITIONS = 4`)
- `AND` / `OR` 다중 조건 지원
- `=`, `!=`, `<`, `>`, `<=`, `>=` 연산자 지원
- 서브쿼리 미지원
- 괄호를 이용한 우선순위 그룹핑 미지원 (왼쪽→오른쪽 순서 평가)

### DISTINCT
- `SELECT DISTINCT * FROM table` — 전체 행 기준 중복 제거 지원
- `SELECT DISTINCT col FROM table` — 특정 컬럼 기준 중복 제거 지원

### LIMIT
- `SELECT * FROM table LIMIT n` — 상위 n개 행만 출력 ✅ 구현 완료
- `SELECT * FROM table WHERE ... LIMIT n` 조합 지원

### ORDER BY
- `SELECT * FROM table ORDER BY col [ASC|DESC]` ✅ 구현 완료
- `qsort` 기반 O(n log n) 정렬
- INT / FLOAT / TEXT 타입 모두 지원
- `WHERE ... ORDER BY ... LIMIT n` 조합 지원

### 고정 스코프 (변경 없음)
- 컬럼 타입: INT / TEXT / FLOAT 세 가지만
- UPDATE: 한 번에 컬럼 1개만 수정
- 파일 포맷: 텍스트(CSV 유사) 고정
- JOIN, GROUP BY, 트랜잭션 미지원
- ORDER BY 다중 컬럼 미지원 (컬럼 1개만)
