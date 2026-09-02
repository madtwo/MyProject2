# -*- coding: utf-8 -*-
"""
Creates Blueprint child assets backed by the GravityShiftCore native classes.

Run inside Unreal Editor:
    Tools > Execute Python Script
Then choose this file.

The script is idempotent and does not overwrite an existing Blueprint whose
parent class differs. In that case it creates a safe BP_GS_* asset under
/Game/GravityShift/NativeStarter instead.
"""
from __future__ import annotations

import json
import os
import unreal

PRIMARY_ROOT = "/Game/GravityShift"
SAFE_ROOT = "/Game/GravityShift/NativeStarter"

ASSET_SPECS = [
    {
        "primary": f"{PRIMARY_ROOT}/Core/BP_GravityManager",
        "safe": f"{SAFE_ROOT}/Core/BP_GS_GravityManager",
        "class_path": "/Script/GravityShiftCore.GSGravityManager",
    },
    {
        "primary": f"{PRIMARY_ROOT}/Components/BPC_GravityBody",
        "safe": f"{SAFE_ROOT}/Components/BPC_GS_GravityBody",
        "class_path": "/Script/GravityShiftCore.GSGravityBodyComponent",
    },
    {
        "primary": f"{PRIMARY_ROOT}/Components/BPC_Breakable",
        "safe": f"{SAFE_ROOT}/Components/BPC_GS_Breakable",
        "class_path": "/Script/GravityShiftCore.GSBreakableComponent",
    },
    {
        "primary": f"{PRIMARY_ROOT}/Components/BPC_Resettable",
        "safe": f"{SAFE_ROOT}/Components/BPC_GS_Resettable",
        "class_path": "/Script/GravityShiftCore.GSResettableComponent",
    },
    {
        "primary": f"{PRIMARY_ROOT}/Blocks/BP_BlockBase",
        "safe": f"{SAFE_ROOT}/Blocks/BP_GS_BlockBase",
        "class_path": "/Script/GravityShiftCore.GSGravityBlock",
    },
    {
        "primary": f"{PRIMARY_ROOT}/Interactions/BP_SurfaceModifierPanel",
        "safe": f"{SAFE_ROOT}/Interactions/BP_GS_SurfaceModifierPanel",
        "class_path": "/Script/GravityShiftCore.GSSurfaceModifierVolume",
    },
    {
        "primary": f"{PRIMARY_ROOT}/Tests/BP_GravityDemoRoom",
        "safe": f"{SAFE_ROOT}/Tests/BP_GS_GravityDemoRoom",
        "class_path": "/Script/GravityShiftCore.GSGravityDemoRoom",
    },
]


def log(message: str) -> None:
    unreal.log(f"[GravityShiftInstaller] {message}")


def warn(message: str) -> None:
    unreal.log_warning(f"[GravityShiftInstaller] {message}")


def ensure_directory(asset_path: str) -> None:
    directory = asset_path.rsplit("/", 1)[0]
    if not unreal.EditorAssetLibrary.does_directory_exist(directory):
        if not unreal.EditorAssetLibrary.make_directory(directory):
            raise RuntimeError(f"Could not create content directory: {directory}")


def class_path(value) -> str:
    if value is None:
        return ""
    try:
        return str(value.get_path_name())
    except Exception:
        return str(value)


def load_native_class(path: str):
    native_class = unreal.load_class(None, path)
    if native_class is None:
        raise RuntimeError(
            f"Native class not found: {path}. "
            "The GravityShiftCore plugin must be enabled and compiled first."
        )
    return native_class


def create_or_reuse_blueprint(asset_path: str, parent_class):
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        asset = unreal.load_asset(asset_path)
        if not isinstance(asset, unreal.Blueprint):
            raise RuntimeError(f"Existing asset is not a Blueprint: {asset_path}")

        existing_parent = unreal.BlueprintEditorLibrary.get_blueprint_parent_class(asset)
        if class_path(existing_parent) != class_path(parent_class):
            return None, (
                f"parent mismatch: existing={class_path(existing_parent)}, "
                f"required={class_path(parent_class)}"
            )

        ok = unreal.BlueprintEditorLibrary.compile_blueprint(asset)
        if not ok:
            raise RuntimeError(f"Existing Blueprint failed to compile: {asset_path}")
        unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
        return asset, "reused"

    ensure_directory(asset_path)
    asset = unreal.BlueprintEditorLibrary.create_blueprint_asset_with_parent(
        asset_path,
        parent_class,
    )
    if asset is None:
        raise RuntimeError(f"Could not create Blueprint: {asset_path}")

    ok = unreal.BlueprintEditorLibrary.compile_blueprint(asset)
    if not ok:
        raise RuntimeError(f"New Blueprint failed to compile: {asset_path}")

    unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
    return asset, "created"


def install() -> dict:
    report = {
        "engine": unreal.BlueprintEditorLibrary.get_current_engine_version(),
        "created": [],
        "reused": [],
        "safe_fallbacks": [],
        "failures": [],
    }

    for spec in ASSET_SPECS:
        try:
            native_class = load_native_class(spec["class_path"])
            asset, status = create_or_reuse_blueprint(spec["primary"], native_class)

            if asset is None:
                warn(f"{spec['primary']} was preserved because of {status}")
                safe_asset, safe_status = create_or_reuse_blueprint(
                    spec["safe"],
                    native_class,
                )
                if safe_asset is None:
                    raise RuntimeError(
                        f"Both primary and safe Blueprint paths have incompatible parents: "
                        f"{spec['primary']} and {spec['safe']}"
                    )
                report["safe_fallbacks"].append(
                    {
                        "primary": spec["primary"],
                        "safe": spec["safe"],
                        "reason": status,
                        "status": safe_status,
                    }
                )
                log(f"{safe_status}: {spec['safe']}")
            else:
                report[status].append(spec["primary"])
                log(f"{status}: {spec['primary']}")

        except Exception as exc:
            entry = f"{spec['primary']}: {exc}"
            report["failures"].append(entry)
            unreal.log_error(f"[GravityShiftInstaller] {entry}")

    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)

    output_path = os.path.join(
        unreal.Paths.project_saved_dir(),
        "GravityShiftNativeInstall.json",
    )
    with open(output_path, "w", encoding="utf-8") as handle:
        json.dump(report, handle, ensure_ascii=False, indent=2)

    report["pass"] = not report["failures"]
    if report["pass"]:
        log(f"PASS. Report: {output_path}")
    else:
        unreal.log_error(
            f"[GravityShiftInstaller] FAIL with {len(report['failures'])} error(s). "
            f"Report: {output_path}"
        )
    return report


if __name__ == "__main__":
    install()
