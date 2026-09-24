# Enemy 콘텐츠

적 개발자가 적별 자식 Blueprint, Flipbook, AI 보조 에셋, 공격 연출을 둡니다. 부모는 `AARBaseEnemy`로 하고 기본 `AARAIController`를 유지합니다.

추적·순찰은 NavMesh와 `AR AI Move To Actor`/`AR AI Move To Location` 또는 Behavior Tree `Move To`를 사용합니다. 공통 AI Controller가 경직·기절·속박 중 경로 이동을 취소합니다. 제한이 풀리면 AI가 목표를 다시 평가해 새 이동을 요청해야 합니다. 일반 방향 이동은 `Request Basic Move`, 공격 중 돌진은 Action Handle을 가진 `Request Action Move`/`Request Action Velocity`를 사용합니다. 모든 공격과 스킬은 `Try Start Action`을 거쳐야 CC 차단·취소 규칙이 적용됩니다.
