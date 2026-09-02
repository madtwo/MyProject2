# -*- coding: utf-8 -*-
"""
Safely reparent blank Blueprint shells created by the original bootstrap script.

Rules:
- A backup copy is created first.
- Blueprints with local variables or implemented functions/events are skipped.
- If compilation fails after reparenting, the original parent is restored.
"""
from __future__ import annotations

import json
import os
import unreal

BACKUP_ROOT = "/Game/GravityShift/_LegacyBackup"

SPECS = [
    ("/Game/GravityShift/Core/BP_GravityManager", "/Script/GravityShiftCore.GSGravityManager"),
    ("/Game/GravityShift/Components/BPC_GravityBody", "/Script/GravityShiftCore.GSGravityBodyComponent"),
    ("/Game/GravityShift/Components/BPC_Breakable", "/Script/GravityShiftCore.GSBreakableComponent"),
    ("/Game/GravityShift/Components/BPC_Resettable", "/Script/GravityShiftCore.GSResettableComponent"),
    ("/Game/GravityShift/Blocks/BP_BlockBase", "/Script/GravityShiftCore.GSGravityBlock"),
    ("/Game/GravityShift/Interactions/BP_SurfaceModifierPanel", "/Script/GravityShiftCore.GSSurfaceModifierVolume"),
]


def info_is_implemented(info) -> bool:
    try:
        return bool(info.get_editor_property("is_implemented"))
    except Exception:
        return False


def get_implemented_members(blueprint) -> list[str]:
    implemented: list[str] = []
    for info in unreal.BlueprintEditorLibrary.list_functions(blueprint):
        if info_is_implemented(info):
            implemented.append(str(info))
    for info in unreal.BlueprintEditorLibrary.list_events(blueprint):
        if info_is_implemented(info):
            implemented.append(str(info))
    return implemented


def native_path(value) -> str:
    try:
        return str(value.get_path_name())
    except Exception:
        return str(value)


def unique_backup_path(source_path: str) -> str:
    name = source_path.rsplit("/", 1)[-1]
    base = f"{BACKUP_ROOT}/{name}_BeforeGravityShiftCore"
    if not unreal.EditorAssetLibrary.does_asset_exist(base):
        return base

    index = 2
    while unreal.EditorAssetLibrary.does_asset_exist(f"{base}_{index}"):
        index += 1
    return f"{base}_{index}"


def main() -> dict:
    report = {
        "engine": unreal.BlueprintEditorLibrary.get_current_engine_version(),
        "upgraded": [],
        "already_correct": [],
        "skipped": [],
        "failed": [],
    }

    if not unreal.EditorAssetLibrary.does_directory_exist(BACKUP_ROOT):
        unreal.EditorAssetLibrary.make_directory(BACKUP_ROOT)

    for asset_path, desired_parent_path in SPECS:
        if not unreal.EditorAssetLibrary.does_asset_exist(asset_path):
            report["skipped"].append({"asset": asset_path, "reason": "missing"})
            continue

        blueprint = unreal.load_asset(asset_path)
        if not isinstance(blueprint, unreal.Blueprint):
            report["failed"].append({"asset": asset_path, "reason": "not a Blueprint"})
            continue

        desired_parent = unreal.load_class(None, desired_parent_path)
        if desired_parent is None:
            report["failed"].append(
                {"asset": asset_path, "reason": f"missing native class {desired_parent_path}"}
            )
            continue

        old_parent = unreal.BlueprintEditorLibrary.get_blueprint_parent_class(blueprint)
        if native_path(old_parent) == native_path(desired_parent):
            report["already_correct"].append(asset_path)
            continue

        local_variables = list(
            unreal.BlueprintEditorLibrary.list_member_variable_names(blueprint, False)
        )
        implemented_members = get_implemented_members(blueprint)
        if local_variables or implemented_members:
            report["skipped"].append(
                {
                    "asset": asset_path,
                    "reason": "contains user-authored members",
                    "local_variables": local_variables,
                    "implemented_members": implemented_members,
                }
            )
            continue

        backup_path = unique_backup_path(asset_path)
        backup = unreal.EditorAssetLibrary.duplicate_asset(asset_path, backup_path)
        if backup is None:
            report["failed"].append(
                {"asset": asset_path, "reason": f"backup failed: {backup_path}"}
            )
            continue

        try:
            unreal.BlueprintEditorLibrary.reparent_blueprint(blueprint, desired_parent)
            if not unreal.BlueprintEditorLibrary.compile_blueprint(blueprint):
                raise RuntimeError("compile failed after reparent")
            unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)
            report["upgraded"].append(
                {"asset": asset_path, "backup": backup_path, "parent": desired_parent_path}
            )
        except Exception as exc:
            try:
                unreal.BlueprintEditorLibrary.reparent_blueprint(blueprint, old_parent)
                unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
                unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)
            except Exception:
                pass
            report["failed"].append({"asset": asset_path, "reason": str(exc)})

    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    output_path = os.path.join(
        unreal.Paths.project_saved_dir(),
        "GravityShiftLegacyUpgrade.json",
    )
    with open(output_path, "w", encoding="utf-8") as handle:
        json.dump(report, handle, ensure_ascii=False, indent=2)

    if report["failed"]:
        unreal.log_error(f"[GravityShiftUpgrade] Completed with failures. {output_path}")
    else:
        unreal.log(f"[GravityShiftUpgrade] Completed. {output_path}")
    return report


if __name__ == "__main__":
    main()
