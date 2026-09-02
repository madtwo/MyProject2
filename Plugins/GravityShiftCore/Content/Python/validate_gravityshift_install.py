# -*- coding: utf-8 -*-
"""Validate that the plugin classes and generated Blueprint children are usable."""
from __future__ import annotations

import json
import os
import unreal

NATIVE_CLASSES = [
    "/Script/GravityShiftCore.GSGravityManager",
    "/Script/GravityShiftCore.GSGravityBodyComponent",
    "/Script/GravityShiftCore.GSBreakableComponent",
    "/Script/GravityShiftCore.GSResettableComponent",
    "/Script/GravityShiftCore.GSGravityBlock",
    "/Script/GravityShiftCore.GSSurfaceModifierVolume",
    "/Script/GravityShiftCore.GSGravityDemoRoom",
    "/Script/GravityShiftCore.GSGravityMathLibrary",
]

BLUEPRINT_CANDIDATES = [
    (
        "/Game/GravityShift/Core/BP_GravityManager",
        "/Game/GravityShift/NativeStarter/Core/BP_GS_GravityManager",
    ),
    (
        "/Game/GravityShift/Blocks/BP_BlockBase",
        "/Game/GravityShift/NativeStarter/Blocks/BP_GS_BlockBase",
    ),
    (
        "/Game/GravityShift/Interactions/BP_SurfaceModifierPanel",
        "/Game/GravityShift/NativeStarter/Interactions/BP_GS_SurfaceModifierPanel",
    ),
    (
        "/Game/GravityShift/Tests/BP_GravityDemoRoom",
        "/Game/GravityShift/NativeStarter/Tests/BP_GS_GravityDemoRoom",
    ),
]


def main() -> dict:
    report = {
        "engine": unreal.BlueprintEditorLibrary.get_current_engine_version(),
        "missing_native_classes": [],
        "missing_blueprint_groups": [],
        "compile_failures": [],
        "validated_assets": [],
    }

    for class_path in NATIVE_CLASSES:
        if unreal.load_class(None, class_path) is None:
            report["missing_native_classes"].append(class_path)

    for candidates in BLUEPRINT_CANDIDATES:
        found = None
        for path in candidates:
            if unreal.EditorAssetLibrary.does_asset_exist(path):
                found = path
                break

        if found is None:
            report["missing_blueprint_groups"].append(list(candidates))
            continue

        asset = unreal.load_asset(found)
        if not isinstance(asset, unreal.Blueprint):
            report["compile_failures"].append(f"{found}: not a Blueprint")
            continue

        if not unreal.BlueprintEditorLibrary.compile_blueprint(asset):
            report["compile_failures"].append(found)
        else:
            report["validated_assets"].append(found)

    report["pass"] = (
        not report["missing_native_classes"]
        and not report["missing_blueprint_groups"]
        and not report["compile_failures"]
    )

    output_path = os.path.join(
        unreal.Paths.project_saved_dir(),
        "GravityShiftNativeValidation.json",
    )
    with open(output_path, "w", encoding="utf-8") as handle:
        json.dump(report, handle, ensure_ascii=False, indent=2)

    if report["pass"]:
        unreal.log(f"[GravityShiftValidation] PASS. {output_path}")
    else:
        unreal.log_error(
            "[GravityShiftValidation] FAIL.\n"
            + json.dumps(report, ensure_ascii=False, indent=2)
        )
    return report


if __name__ == "__main__":
    main()
