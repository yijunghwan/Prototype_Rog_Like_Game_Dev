"""Prepare unbound button Input Actions without modifying existing assets.

Run with Unreal's PythonScript commandlet. This deliberately does not edit
mapping contexts, player Blueprints, Gameplay Tags, or consumable inputs.
"""

import unreal


PACKAGE_PATH = "/Game/Game/Tests/Player/Skill_Input"
KEY_LABELS = ("Shift", "Q", "E", "R", "G", "Z", "X", "C", "F")


def main():
    assets = unreal.EditorAssetLibrary
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    created = 0
    for key_label in KEY_LABELS:
        name = "IA_Key_" + key_label
        path = PACKAGE_PATH + "/" + name
        if assets.does_asset_exist(path):
            existing = assets.load_asset(path)
            if not isinstance(existing, unreal.InputAction):
                raise RuntimeError("Existing asset is not an Input Action: " + path)
            unreal.log("INPUT_PREP preserved existing: " + path)
            continue

        action = asset_tools.create_asset(
            name, PACKAGE_PATH, unreal.InputAction, unreal.InputAction_Factory()
        )
        if not action:
            raise RuntimeError("Could not create Input Action: " + path)
        action.set_editor_property("value_type", unreal.InputActionValueType.BOOLEAN)
        if not assets.save_loaded_asset(action, only_if_is_dirty=False):
            raise RuntimeError("Could not save Input Action: " + path)
        if action.get_editor_property("value_type") != unreal.InputActionValueType.BOOLEAN:
            raise RuntimeError("Unexpected Input Action value type: " + path)
        created += 1
        unreal.log("INPUT_PREP created unbound bool action: " + path)

    for key_label in KEY_LABELS:
        path = PACKAGE_PATH + "/IA_Key_" + key_label
        if not assets.does_asset_exist(path):
            raise RuntimeError("Verification failed; asset missing: " + path)
    unreal.log("INPUT_PREP complete: {} created; existing assets preserved".format(created))


main()
