# TODO List

## 마일스톤

| 회차 | 목표 | 상태 |
|------|------|------|
|   ~ 7 | REPL + 파서 — 명령어 입력/분기 동작 | 완료 |
| 7 ~ 8 | DB 구조체 + 연산 — 메모리에 데이터 저장/조회 | 진행 중 |
|   ~ 8 | 해시테이블 — PK O(1) 조회 | 미시작 |
| 8 ~ 9 | 파서 ↔ DB 연결 — 명령어 실행 시 실제 데이터 변경 | 미시작 |
|   ~ 9 | 파일 저장/로드 — 데이터 영속성 | 미시작 |
|   ~ 9 | 마무리 — 메모리 누수 검증, 테스트 | 미시작 |

---

## 진행 현황

| 파일 | 상태 | 설명 |
|------|------|------|
| `main.c` | 완료 | REPL 루프, 배너 출력 |
| `parser.h / parser.c` | 완료 | tokenize, dispatch, parse_* |
| `db.h` | 완료 | 구조체 정의 (Cell, Row, Column, Table, Database) |
| `db.c` | 진행 중 | DB 연산 함수 구현 |
| `hashtable.h / hashtable.c` | 미시작 | 해시테이블 구현 |
| `storage.h / storage.c` | 미시작 | save / load 파일 I/O |
| parser ↔ db 연결 | 미시작 | parse_* 함수에서 db.c 함수 호출 |

---

### 1. db.c 완성
- [ ] `db_create` — Database malloc + 초기화
- [ ] `table_find` — 이름으로 테이블 탐색
- [ ] `table_create` — 테이블 생성 + db linked list에 연결
- [ ] `table_drop` — 테이블 + 모든 Row 메모리 해제
- [ ] `row_insert` — Row malloc + cells 복사 + TEXT strdup
- [ ] `row_find` — WHERE 조건으로 Row 찾기 (PK면 O(1), 아니면 O(n))
- [ ] `row_update` — WHERE 조건에 맞는 모든 Row의 특정 컬럼 Cell 값 교체
- [ ] `row_delete` — Row list에서 제거 + 메모리 해제
- [ ] `db_free` — 전체 메모리 해제 (종료 시 호출)

### 2. hashtable.c 구현
- [ ] `ht_create(int bucket_count)` — 버킷 배열 malloc
- [ ] `ht_insert(HashTable*, Cell* pk, Row*)` — 해시 계산 후 버킷에 연결
- [ ] `ht_find(HashTable*, Cell* pk)` — O(1) 조회
- [ ] `ht_delete(HashTable*, Cell* pk)` — 버킷에서 제거
- [ ] `ht_free(HashTable*)` — 버킷 배열 해제
- [ ] `row_insert` / `row_delete`에 해시테이블 연동

### 3. parser ↔ db 연결
- [ ] `main.c`에서 `db_create()` 호출, DB 인스턴스 생성
- [ ] `parse_command()`에 DB 포인터 전달
- [ ] `parse_create` → `table_create` 호출
- [ ] `parse_drop` → `table_drop` 호출
- [ ] `parse_insert` → `row_insert` 호출 (값 파싱 + 타입 변환 포함)
- [ ] `parse_select` → WHERE 없으면 전체 출력, WHERE 있으면 `row_find_where` 호출, DISTINCT 있으면 `rows_distinct` 통해 중복 제거 후 출력
- [ ] `rows_distinct` — 결과 Row 목록에서 중복 행 제거
- [ ] `parse_update` → `row_find`로 Row 찾은 뒤 `row_update` 호출
- [ ] `parse_delete` → `row_delete` 호출
- [ ] `parse_show` → 테이블 목록 출력
- [ ] `parse_describe` → 컬럼 목록 출력

### 4. storage.c 구현
- [ ] `db_save(Database*, const char* path)` — 전체 DB를 텍스트로 저장
- [ ] `db_load(Database*, const char* path)` — 파일 읽어서 DB 복원
- [ ] 파일 포맷 (텍스트, CSV 유사):
  ```
  TABLE users
  SCHEMA id INT PK, name TEXT, email TEXT
  1,'Alice','alice@example.com'
  END TABLE
  ```

### 5. 마무리
- [ ] `main.c` 종료 시 `db_free()` 호출
- [ ] `_CrtDumpMemoryLeaks()` 추가 (메모리 누수 확인)
- [ ] TC-01 ~ TC-05 테스트 시나리오 수동 실행
- [ ] `HELP` 명령어 출력 내용 완성

---

## 구현 권장 순서

```
db.c 완성 → hashtable.c → parser↔db 연결 → storage.c → 마무리
```

---

## 스코프 및 한계

### WHERE 문 한계
- 단일 조건만 지원 (`WHERE col = val`)
- `AND` / `OR` 다중 조건 미지원
- `=` 연산자만 지원 — `>`, `<`, `>=`, `<=`, `!=`, `LIKE` 미지원
- 서브쿼리 미지원

### DISTINCT
- `SELECT DISTINCT * FROM table` — 전체 행 기준 중복 제거 지원
- `SELECT DISTINCT col FROM table` — 특정 컬럼 기준 중복 제거 지원

### 변동 가능 항목
- WHERE 연산자 확장 (`>`, `<` 등) — 현재 구현 후 여유가 있으면 추가
- `ORDER BY` 정렬 — 미구현, 출력 순서는 삽입 역순 (linked list 특성)
- 컬럼 수 상한 (`MAX_COLUMNS`) — 현재 상수로 고정, 필요 시 조정

### 고정 스코프 (변경 없음)
- 컬럼 타입: INT / TEXT / FLOAT 세 가지만
- UPDATE: 한 번에 컬럼 1개만 수정
- 파일 포맷: 텍스트(CSV 유사) 고정
- JOIN, GROUP BY, 트랜잭션 미지원
