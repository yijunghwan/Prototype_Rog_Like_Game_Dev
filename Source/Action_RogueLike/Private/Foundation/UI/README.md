# UI 구현

HUD 데이터 스냅샷과 UI 이벤트 전달을 위한 내부 구현을 둡니다. Widget은 생성 시 `GetHUDSnapshot`으로 초기화하고 이후 `OnHUDSnapshotChanged`를 구독합니다. Widget 레이아웃은 Content의 UMG가 담당합니다.
