"""Undo only the mistaken key-named Input Action preparation.

Run in the existing editor to preserve in-memory references and user edits.
The default is a read-only audit. Gameplay Tags are never modified.
"""

import unreal


EXECUTE = False
ROOT = "/Game/Game/Tests/Player/Skill_Input"
ADDED_KEYS = ("Shift", "Q", "E", "R", "F", "G", "Z", "X", "C")
RENAMES = (("IA_Key_MouseLeft", "IA_Mouse_Left"),
           ("IA_Key_MouseRight", "IA_Mouse_Right"))
REFERENCERS = ("/Game/Game/Tests/Player/IMC_Player",
               "/Game/Game/Tests/Player/IMC_Player_MouseDash",
               "/Game/Game/Tests/Player/BP_test_Player")


def main():
    library = unreal.EditorAssetLibrary
    for path in REFERENCERS:
        if not library.load_asset(path):
            raise RuntimeError("Could not load existing input referencer: " + path)
    deletions = []
    for key in ADDED_KEYS:
        path = ROOT + "/IA_Key_" + key
        if not library.does_asset_exist(path):
            continue
        asset = library.load_asset(path)
        if not isinstance(asset, unreal.InputAction):
            raise RuntimeError("Deletion target is not an Input Action: " + path)
        refs = list(library.find_package_referencers_for_asset(path, True))
        unreal.log("INPUT_RESTORE audit: {} references={}".format(path, refs))
        if refs:
            raise RuntimeError("Refusing to delete a referenced Input Action: " + path)
        deletions.append(path)
    requests = []
    renamed = []
    for current_name, original_name in RENAMES:
        current_path = ROOT + "/" + current_name
        original_path = ROOT + "/" + original_name
        if not library.does_asset_exist(current_path):
            original = library.load_asset(original_path)
            if not isinstance(original, unreal.InputAction):
                raise RuntimeError("Original mouse Input Action missing: " + original_path)
            continue
        asset = library.load_asset(current_path)
        if not isinstance(asset, unreal.InputAction):
            raise RuntimeError("Mouse asset is not an Input Action: " + current_path)
        if library.does_asset_exist(original_path) and library.load_asset(original_path) != asset:
            raise RuntimeError("Original name is occupied by a different asset: " + original_path)
        requests.append(unreal.AssetRenameData(asset=asset, new_package_path=ROOT,
                                               new_name=original_name))
        renamed.append((asset, original_path, asset.get_editor_property("value_type")))
        unreal.log("INPUT_RESTORE audit rename: {} -> {}".format(current_path, original_path))
    if not EXECUTE:
        unreal.log("INPUT_RESTORE audit complete: {} deletions, {} renames; no changes".format(
            len(deletions), len(requests)))
        return
    if requests and not unreal.AssetToolsHelpers.get_asset_tools().rename_assets(requests):
        raise RuntimeError("Mouse Input Action restoration failed")
    for asset, path, previous_type in renamed:
        if asset.get_path_name().split(".")[0] != path:
            raise RuntimeError("Mouse action was not restored to " + path)
        if asset.get_editor_property("value_type") != previous_type:
            raise RuntimeError("Mouse action settings changed unexpectedly")
        if not library.save_loaded_asset(asset, only_if_is_dirty=False):
            raise RuntimeError("Could not save restored action: " + path)
        unreal.log("INPUT_RESTORE restored: " + path)
    # Save the existing users after Unreal updates their references. Their
    # current settings are retained; no array or mapping values are reassigned.
    for path in REFERENCERS:
        if not library.save_loaded_asset(library.load_asset(path), only_if_is_dirty=True):
            raise RuntimeError("Could not save existing input referencer: " + path)
    for path in deletions:
        if not library.delete_asset(path):
            raise RuntimeError("Could not delete unreferenced action: " + path)
        if library.does_asset_exist(path):
            raise RuntimeError("Deleted action remains registered: " + path)
        unreal.log("INPUT_RESTORE deleted: " + path)
    unreal.log("INPUT_RESTORE complete; original actions and input tags preserved")


main()
