# Mini-SQLite 명령어 명세

## DDL (테이블 정의)

| 명령어 | 문법 | 설명 |
|--------|------|------|
| CREATE TABLE | `CREATE TABLE name (col TYPE [PRIMARY KEY], ...)` | 테이블 생성 |
| DROP TABLE | `DROP TABLE name` | 테이블 삭제 |
| SHOW TABLES | `SHOW TABLES` | 전체 테이블 목록 |
| DESCRIBE | `DESCRIBE name` | 테이블 스키마 조회 |

## DML (데이터 조작)

| 명령어 | 문법 | 설명 |
|--------|------|------|
| INSERT | `INSERT INTO name VALUES (val, ...)` | 행 삽입 |
| SELECT | `SELECT * FROM name` | 전체 조회 |
| SELECT | `SELECT * FROM name WHERE col = val` | 조건 조회 |
| UPDATE | `UPDATE name SET col = val WHERE col = val` | 행 수정 (컬럼 1개) |
| DELETE | `DELETE FROM name WHERE col = val` | 행 삭제 |

## 유틸리티

| 명령어 | 문법 | 설명 |
|--------|------|------|
| SAVE | `SAVE path` | DB를 파일로 저장 |
| LOAD | `LOAD path` | 파일에서 DB 로드 |
| HELP | `HELP` | 명령어 목록 출력 |
| EXIT | `EXIT` 또는 `QUIT` | 프로그램 종료 |

## 컬럼 타입

| 타입 | 예시 |
|------|------|
| INT | `42` |
| FLOAT | `3.14` |
| TEXT | `'Alice'` |

## 규칙

- 키워드는 대소문자 무관 (`CREATE` = `create`)
- TEXT 값은 작은따옴표로 감싸기
- WHERE 조건은 단일 조건만 (`AND`/`OR` 미지원)
- 잘못된 입력 → `error: 메시지` 출력 후 계속 실행
