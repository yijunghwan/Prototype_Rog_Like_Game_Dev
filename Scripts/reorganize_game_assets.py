"""One-time Unreal Editor migration for test assets into /Game/Game/Tests.

Run with the Python Editor Script commandlet so Unreal updates asset references.
The script never overwrites an asset at its destination.
"""

import unreal


MOVES = (
    ("/Game/Game/Foundation/Test/test_Player/IA_Move", "/Game/Game/Tests/Player/IA_Move"),
    ("/Game/Game/Foundation/Test/test_Player/IA_Roll", "/Game/Game/Tests/Player/IA_Roll"),
    ("/Game/Game/Foundation/Test/test_Player/IA_Roll_v2", "/Game/Game/Tests/Player/IA_Roll_v2"),
    ("/Game/Game/Foundation/Test/test_Player/IMC_Player", "/Game/Game/Tests/Player/IMC_Player"),
    ("/Game/Game/Foundation/Test/test_Player/IMC_Player_MouseDash", "/Game/Game/Tests/Player/IMC_Player_MouseDash"),
    ("/Game/Game/Foundation/Test/test_Player/BP_test_Player", "/Game/Game/Tests/Player/BP_test_Player"),
    ("/Game/Game/Foundation/UI/WBP_TestResourceHUD", "/Game/Game/Tests/UI/WBP_TestResourceHUD"),
    ("/Game/Game/Foundation/Blueprints/BP_TestGameMode", "/Game/Game/Tests/Blueprints/BP_TestGameMode"),
    # L_FoundationTest and NewMap were legacy redirectors, not actual maps.
    ("/Game/Game/Foundation/Test/Test_Level", "/Game/Game/Tests/Maps/Test_Level"),
)


def main():
    assets = unreal.EditorAssetLibrary
    pending = []
    for source, destination in MOVES:
        if assets.does_asset_exist(destination):
            unreal.log("CONTENT_REORG already moved: " + destination)
            continue
        if not assets.does_asset_exist(source):
            raise RuntimeError("Source asset missing: " + source)
        pending.append((source, destination))

    unreal.log("CONTENT_REORG preflight passed: {} pending assets".format(len(pending)))
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    for source, destination in pending:
        if source.endswith("/Test_Level"):
            # This map is held by an editor settings CDO. A headless rename asks
            # for a modal confirmation, so save the loaded world at its new path.
            world = unreal.EditorLoadingAndSavingUtils.load_map(source)
            if not world or not unreal.EditorLoadingAndSavingUtils.save_map(world, destination):
                raise RuntimeError("Unreal failed to save the test map at " + destination)
            unreal.log("CONTENT_REORG saved map {} -> {}".format(source, destination))
            continue
        asset = assets.load_asset(source)
        if not asset:
            raise RuntimeError("Could not load source asset: " + source)
        package_path, _, name = destination.rpartition("/")
        request = unreal.AssetRenameData(asset=asset, new_package_path=package_path, new_name=name)
        if not tools.rename_assets([request]):
            raise RuntimeError("Unreal failed to move {} to {}".format(source, destination))
        unreal.log("CONTENT_REORG moved {} -> {}".format(source, destination))

    if not unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True):
        raise RuntimeError("Unreal did not save all changed packages")
    for _, destination in MOVES:
        if not assets.does_asset_exist(destination):
            raise RuntimeError("Moved asset missing: " + destination)
    unreal.log("CONTENT_REORG complete: {} assets".format(len(MOVES)))


main()
