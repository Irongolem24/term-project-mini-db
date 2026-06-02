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
|   ~ 9 | 해시테이블 — PK O(1) 조회 | 완료 |

---

## 진행 현황

| 파일 | 상태 | 설명 |
|------|------|------|
| `main.c` | 완료 | REPL 루프, 자동 save/load, _CrtDumpMemoryLeaks |
| `parser.h / parser.c` | 완료 | tokenize, dispatch, parse_* 전체 + ORDER BY / LIMIT |
| `db.h` | 완료 | 구조체 정의 + WhereClause, Operator, Condition |
| `db.c` | 완료 | DB 연산 함수 전체 구현 |
| `storage.h / storage.c` | 완료 | 자동 save / load 파일 I/O |
| `hashtable.h / hashtable.c` | 완료 | PK → Row* 해시테이블 (chaining) |

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

### 5. 해시테이블 (hashtable.c)
- [x] `ht_create(int bucket_count)` — 버킷 배열 calloc
- [x] `ht_insert(HashTable*, Cell* pk, Row*)` — djb2 해시 후 버킷 앞에 연결 (chaining)
- [x] `ht_find(HashTable*, Cell* pk)` — O(1) 조회
- [x] `ht_delete(HashTable*, Cell* pk)` — 버킷에서 제거 + TEXT 키 해제
- [x] `ht_free(HashTable*)` — 전체 엔트리 + 버킷 배열 해제
- [x] `table_create` / `table_drop`에 ht 생성·해제 연동
- [x] `row_insert` / `row_delete` / `rows_delete_where`에 ht 등록·제거 연동
- [x] `row_find_by_pk` → `ht_find` O(1) 조회로 교체
- [x] `rows_update_where`에서 PK 컬럼 수정 시 ht 키 갱신 (delete+insert)
- [x] `ht_resize` — load factor 0.75 초과 시 버킷 2배 확장 + rehash (O(1) 유지)

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

### 해시테이블
- 초기 버킷 `HT_BUCKETS = 64`, 충돌은 chaining(linked list)으로 처리
- **동적 확장(rehash)**: load factor 0.75 초과 시 버킷 2배로 늘려 재배치 → 대량 데이터에서도 O(1) 유지
- PK 조회(`row_find_by_pk`)는 O(1) — INSERT 중복 체크 / `row_delete`에서 사용
- INT / FLOAT / TEXT PK 모두 지원 (djb2 해시, TEXT 키는 별도 복사·해제)
- WHERE 기반 SELECT/DELETE/UPDATE는 여전히 O(n) 선형 탐색 (해시는 PK 등호 조회 전용)
- 성능 측정: N=80k 기준 rehash 적용 후 PK 조회 per-op ~2.9배 단축, 총 빌드 시간 선형 스케일 확인

### 고정 스코프 (변경 없음)
- 컬럼 타입: INT / TEXT / FLOAT 세 가지만
- UPDATE: 한 번에 컬럼 1개만 수정
- 파일 포맷: 텍스트(CSV 유사) 고정
- JOIN, GROUP BY, 트랜잭션 미지원
- ORDER BY 다중 컬럼 미지원 (컬럼 1개만)
