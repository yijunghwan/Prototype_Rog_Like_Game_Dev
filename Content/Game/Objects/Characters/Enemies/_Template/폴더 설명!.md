# 적 콘텐츠 폴더 템플릿

새 적은 `Enemies/[적ID]/`를 만들고 아래 구조를 복사해 사용합니다.

```text
[적ID]/
├─ Art/Textures/ Sprites/ Flipbooks/ Materials/ FX/ Icons/
├─ Blueprints/
├─ Data/
└─ AI/                 ← 실제 AI 에셋이 필요할 때만
```

적별 Sprite Sheet는 `Art/Textures`, 추출 Sprite는 `Art/Sprites`, Flipbook은 `Art/Flipbooks`에 둡니다. 적 공격은 반드시 Foundation Action Handle 규약을 사용합니다.
