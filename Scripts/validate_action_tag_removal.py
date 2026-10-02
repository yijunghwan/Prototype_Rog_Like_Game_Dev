"""Load saved game content and compile Blueprints without saving any assets.

Run with Unreal's PythonScript commandlet after a full editor build.
"""

import unreal


def main():
    request = unreal.ARActionRequest()
    request.get_editor_property("cancel_rules")
    try:
        request.get_editor_property("action_tag")
    except Exception as error:
        if "Failed to find property" not in str(error):
            raise
    else:
        raise RuntimeError("Removed Action Tag property is still exposed")

    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    registry.search_all_assets(synchronous_search=True)
    blueprint_count = 0
    item_count = 0
    for data in registry.get_assets_by_path("/Game/Game", recursive=True):
        class_name = str(data.asset_class_path.asset_name)
        if class_name.endswith("Blueprint"):
            blueprint = data.get_asset()
            if not blueprint or not isinstance(blueprint, unreal.Blueprint):
                raise RuntimeError("Could not load Blueprint: " + str(data.package_name))
            if not unreal.BlueprintEditorLibrary.compile_blueprint(blueprint):
                raise RuntimeError("Blueprint compilation failed: " + str(data.package_name))
            unreal.log("ACTION_TAG_REMOVAL blueprint verified: " + str(data.package_name))
            blueprint_count += 1
        elif class_name == "ARItemDefinition":
            definition = data.get_asset()
            if not definition:
                raise RuntimeError("Could not load saved item definition: " + str(data.package_name))
            for skill in definition.get_editor_property("skill_definitions"):
                skill.get_editor_property("action_request").get_editor_property("cancel_rules")
                unreal.log("ACTION_TAG_REMOVAL saved skill loaded: {} / {}".format(
                    data.package_name, skill.get_editor_property("skill_id")))
            item_count += 1
    if blueprint_count == 0 or item_count == 0:
        raise RuntimeError("No game Blueprints or item definitions were checked")
    unreal.log("ACTION_TAG_REMOVAL content verified: {} Blueprints, {} item definitions; no assets saved".format(
        blueprint_count, item_count))


main()
