# Weapons

무기마다 하위 폴더를 만들고 `Data`, `Runtime`, `Art`, 필요 시 `Projectiles`를 둡니다. 무기의 정적 정보는 Data Asset, 실제 동작은 Runtime Blueprint에 둡니다.

```text
[무기ID]/
├─ Data/                         ← Weapon Definition Data Asset
├─ Runtime/                      ← Runtime Blueprint
├─ Art/Textures/ Sprites/ Flipbooks/ FX/ Icons/
└─ Projectiles/                  ← 투사체가 필요할 때만
```

원본 Sprite Sheet와 작업 파일은 `ArtSource/Weapons/[무기ID]`에 둡니다.
