"""Read saved test-player input routes without modifying or saving assets."""
import unreal

root = "/Game/Game/Tests/Player/"
key_name_methods = [name for name in dir(unreal.InputLibrary)
                    if "display_name" in name and "key" in name and "chord" not in name]
unreal.log("KEY_ROUTE key_name_methods={}".format(key_name_methods))
if len(key_name_methods) != 1:
    raise RuntimeError("Expected one key display name method")
key_display_name = getattr(unreal.InputLibrary, key_name_methods[0])
for path in unreal.EditorAssetLibrary.list_assets(root, recursive=True, include_folder=False):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if isinstance(asset, unreal.InputMappingContext):
        for mapping in asset.get_editor_property("default_key_mappings").get_editor_property("mappings"):
            unreal.log("KEY_ROUTE context={} action={} key={}".format(
                path, mapping.get_editor_property("action"), key_display_name(mapping.get_editor_property("key"))))
    elif isinstance(asset, unreal.Blueprint):
        cls = unreal.BlueprintEditorLibrary.generated_class(asset)
        if not cls:
            continue
        defaults = unreal.get_default_object(cls)
        for prop in ("player_mapping_context", "default_mapping_context", "active_relic_slot_input_bindings"):
            try:
                value = defaults.get_editor_property(prop)
            except Exception:
                continue
            unreal.log("KEY_ROUTE defaults={} property={} value={}".format(path, prop, value))
            if prop == "active_relic_slot_input_bindings":
                for binding in value:
                    unreal.log("KEY_ROUTE SLOT action={} slot={} skill={}".format(
                        binding.get_editor_property("input_action"),
                        binding.get_editor_property("active_relic_slot"),
                        binding.get_editor_property("skill_index")))
controller = unreal.EditorAssetLibrary.load_asset("/Game/Game/Foundation/Blueprints/BP_PlayerController")
if controller:
    cls = unreal.BlueprintEditorLibrary.generated_class(controller)
    unreal.log("KEY_ROUTE controller_context={}".format(
        unreal.get_default_object(cls).get_editor_property("player_mapping_context")))
unreal.log("KEY_ROUTE inspection complete; no assets modified or saved")
