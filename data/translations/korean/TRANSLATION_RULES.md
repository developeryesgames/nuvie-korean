# Ultima 6 Korean Translation Rules

## 파일 구조

- `dialogues_ko_original.txt` - 원본 (절대 수정 금지)
- `dialogues_ko.txt` - 번역 작업 파일

## 형식 규칙

### DESC (NPC 설명) - 따옴표 없음
```
NPC번호|영어 설명|한국어 설명
```
예시:
```
3|a quiet man, who almost seems to be a creature of the forest.|조용한 남자로, 거의 숲의 생물처럼 보인다.
62|the druidess Jaana.|드루이드 자나다.
```

### 대사 - 따옴표 있음
```
NPC번호|"영어 대사"|"한국어 대사"
```
예시:
```
3|"Yes, my friend?"|"그래, 친구?"
3|"A pleasure."|"즐거웠소."
```

## 번역 상태

- `<번역 필요>` - 미번역
- 한국어 텍스트 - 번역 완료

## 변수

- `$P` - 플레이어 이름
- `$G` - 성별 호칭 (Avatar/Lord/Lady)
- `$T` - 시간대 (morning/afternoon/evening)
- `$N` - NPC 이름
- `$Z` - 입력값

## 주의사항

1. 원본 영어 텍스트는 절대 수정하지 않음
2. DESC와 대사의 따옴표 규칙 엄수
3. `@키워드`는 그대로 유지 (하이라이트용)
