# Status Effect Data

Stun, Root와 이후 공통 상태이상의 아이콘·표시·기본 데이터를 둡니다. 실제 상태 판정과 지속시간 계산은 C++ Component가 담당합니다.

현재 실제 Data Asset은 아직 없습니다. `UARStatusEffectDefinition` 기반으로 아래처럼 만듭니다.

| 에셋 | Status Tag | Blocks Basic Movement | Blocks All Movement | Blocks Roll | Blocks Skill Groups | Cancel Actions On Apply |
|---|---|---|---|---|---|---|
| `DA_Status_Root` | `Status.Root` | 켜기 | 끄기 | 켜기 | 끄기 | 끄기 (진행 중인 롤은 자동 취소) |
| `DA_Status_Stun` | `Status.Stun` | 켜기 | 켜기 | 켜기 | 켜기 | 켜기, 사유 `Stun` |

두 효과의 `Affected by Tenacity`는 게임 디자인에 따라 정합니다. 강인함 100을 면역처럼 쓸 계획이면 켭니다. 공격 적중 시 `Apply Status Effect`로 대상에게 적용합니다. 진행 중인 행동의 실제 취소 여부는 그 행동의 취소 규칙에도 따릅니다.
