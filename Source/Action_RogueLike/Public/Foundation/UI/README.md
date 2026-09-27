# UI

화면 종류와 HUD·인벤토리가 구독할 읽기 전용 UI 타입을 둡니다. `FARPlayerHUDSnapshot`은 체력·보호막·MP·스태미나·조준·장착 아이템·스킬·소모품의 현재 표시값을 한 번에 전달합니다. 화면 점유·입력 전환은 `UARUIManagerComponent`가 담당하고, UMG Widget과 실제 디자인은 Content 폴더에 둡니다.

`UARResourceHUDWidget`은 임시 자원 HUD의 데이터 연결을 담당합니다. `WBP_TestResourceHUD`의 Designer 트리에서 이름으로 바·텍스트를 연결하므로 외형은 콘텐츠 에셋에서 편집합니다. `AARPlayerController`가 로컬 HUD를 생성하고 소유 Pawn 변경·종료 시 구독을 정리합니다. `UARCreateResourceHUDCommandlet`은 에디터 빌드에서만 초기 에셋을 생성하며 기존 파일을 보존합니다.
