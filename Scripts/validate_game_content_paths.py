"""Read-only validation for the moved test content."""

import unreal


ASSETS = (
    "/Game/Game/Tests/Player/BP_test_Player",
    "/Game/Game/Tests/Player/IMC_Player",
    "/Game/Game/Tests/UI/WBP_TestResourceHUD",
    "/Game/Game/Tests/Blueprints/BP_TestGameMode",
)
MAPS = ("/Game/Game/Tests/Maps/Test_Level",)


for path in ASSETS + MAPS:
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not asset:
        raise RuntimeError("Could not load asset: " + path)
    unreal.log("CONTENT_REORG_VALID {} class={}".format(path, asset.get_class().get_name()))

for path in MAPS:
    world = unreal.EditorLoadingAndSavingUtils.load_map(path)
    if not world:
        raise RuntimeError("Could not open map: " + path)
    if world.get_path_name() != path + "." + path.rsplit("/", 1)[-1]:
        raise RuntimeError("Map resolved to unexpected asset: " + world.get_path_name())
    unreal.log("CONTENT_REORG_MAP_OPEN {} world={}".format(path, world.get_path_name()))

unreal.log("CONTENT_REORG_VALIDATION_COMPLETE")
