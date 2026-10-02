"""Refresh the new config-backed input tags in the currently running editor.

The INI is edited separately; this only synchronizes the settings in memory.
Never changes item definitions, Input Actions, or player input bindings.
"""

import unreal


KEY_LABELS = ("MouseLeft", "MouseRight", "Shift", "Q", "E", "R", "F", "G", "Z", "X", "C")


def main():
    settings_class = unreal.load_class(None, "/Script/GameplayTags.GameplayTagsSettings")
    if not settings_class:
        raise RuntimeError("Could not load Gameplay Tags settings class")
    settings = unreal.get_default_object(settings_class)
    rows = list(settings.get_editor_property("GameplayTagList"))
    registered = {str(row.get_editor_property("Tag")) for row in rows}
    for label in KEY_LABELS:
        name = "Input.Skill." + label
        if name not in registered:
            row = unreal.GameplayTagTableRow()
            row.set_editor_property("Tag", unreal.Name(name))
            row.set_editor_property("DevComment", "Shared input request; actual key mapping is configured separately.")
            rows.append(row)
            registered.add(name)
    # SetEditorProperty invokes the normal settings-change notification, which
    # refreshes the Gameplay Tags tree through GameplayTagsEditor.
    settings.set_editor_property("GameplayTagList", rows)
    current = {str(row.get_editor_property("Tag")) for row in settings.get_editor_property("GameplayTagList")}
    for label in KEY_LABELS:
        name = "Input.Skill." + label
        if name not in current:
            raise RuntimeError("Input tag missing from editor settings: " + name)
        unreal.log("INPUT_TAGS verified: " + name)
    unreal.log("INPUT_TAGS complete: 11 shared key-named tags; bindings unchanged")


main()
