"""Rename mouse Input Actions through the running editor, preserving references.

Save only the renamed actions and their known input referencers, preserving
current editor changes. Do not change mappings, bindings, or Gameplay Tags.
"""

import unreal


PACKAGE_PATH = "/Game/Game/Tests/Player/Skill_Input"
RENAMES = (
    ("IA_Mouse_Left", "IA_Key_MouseLeft"),
    ("IA_Mouse_Right", "IA_Key_MouseRight"),
)


def main():
    assets = unreal.EditorAssetLibrary
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    reference_paths = (
        "/Game/Game/Tests/Player/IMC_Player",
        "/Game/Game/Tests/Player/IMC_Player_MouseDash",
        "/Game/Game/Tests/Player/BP_test_Player",
    )
    # Load the main users before renaming so their in-memory references follow
    # the same objects, including any unsaved changes in the running editor.
    for path in reference_paths:
        if not assets.load_asset(path):
            raise RuntimeError("Could not load input reference asset: " + path)

    requests = []
    tracked = []
    for old_name, new_name in RENAMES:
        old_path = PACKAGE_PATH + "/" + old_name
        new_path = PACKAGE_PATH + "/" + new_name
        if assets.does_asset_exist(new_path):
            action = assets.load_asset(new_path)
            if not isinstance(action, unreal.InputAction):
                raise RuntimeError("Rename destination is not an Input Action: " + new_path)
            if assets.does_asset_exist(old_path) and assets.load_asset(old_path) != action:
                raise RuntimeError("Distinct assets occupy both rename paths: " + old_path)
            unreal.log("INPUT_RENAME already renamed: " + new_path)
        else:
            action = assets.load_asset(old_path)
            if not isinstance(action, unreal.InputAction):
                raise RuntimeError("Mouse Input Action missing: " + old_path)
            requests.append(unreal.AssetRenameData(
                asset=action, new_package_path=PACKAGE_PATH, new_name=new_name
            ))
        tracked.append((action, new_path, action.get_editor_property("value_type")))

    if requests and not tools.rename_assets(requests):
        raise RuntimeError("Unreal did not complete mouse Input Action renaming")
    for action, new_path, previous_type in tracked:
        if action.get_path_name().split(".")[0] != new_path:
            raise RuntimeError("Unexpected renamed asset path: " + action.get_path_name())
        if action.get_editor_property("value_type") != previous_type:
            raise RuntimeError("Renaming changed the Input Action value type")
        if not assets.save_loaded_asset(action, only_if_is_dirty=False):
            raise RuntimeError("Could not save renamed Input Action: " + new_path)
        unreal.log("INPUT_RENAME verified: " + new_path)

    mouse_actions = [entry[0] for entry in tracked]
    for path in reference_paths[:2]:
        context = assets.load_asset(path)
        uses_mouse = False
        mapping_data = context.get_editor_property("default_key_mappings")
        for mapping in mapping_data.get_editor_property("mappings"):
            action = mapping.get_editor_property("action")
            if action in mouse_actions:
                uses_mouse = True
                unreal.log("INPUT_RENAME mapping preserved: {} -> {}".format(
                    mapping.get_editor_property("key"), action.get_path_name()
                ))
        if uses_mouse and not assets.save_loaded_asset(context, only_if_is_dirty=True):
            raise RuntimeError("Could not save updated input reference: " + path)

    player_class = assets.load_blueprint_class(reference_paths[2])
    if not player_class:
        raise RuntimeError("Could not inspect the player input bindings")
    player_defaults = unreal.get_default_object(player_class)
    uses_mouse = False
    for binding in player_defaults.get_editor_property("skill_input_bindings"):
        action = binding.get_editor_property("input_action")
        if action in mouse_actions:
            uses_mouse = True
            unreal.log("INPUT_RENAME player binding preserved: " + action.get_path_name())
    if uses_mouse and not assets.save_loaded_asset(
        assets.load_asset(reference_paths[2]), only_if_is_dirty=True
    ):
        raise RuntimeError("Could not save updated player input references")
    unreal.log("INPUT_RENAME complete; existing input settings preserved")


main()
