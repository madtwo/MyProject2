# -*- coding: utf-8 -*-
"""Spawn the generated Gravity Shift demo room into the currently open level."""
from __future__ import annotations

import unreal

CANDIDATE_ASSETS = [
    "/Game/GravityShift/Tests/BP_GravityDemoRoom",
    "/Game/GravityShift/NativeStarter/Tests/BP_GS_GravityDemoRoom",
]


def generated_class_path(asset_path: str) -> str:
    asset_name = asset_path.rsplit("/", 1)[-1]
    return f"{asset_path}.{asset_name}_C"


def main():
    demo_class = None
    source_path = None

    for asset_path in CANDIDATE_ASSETS:
        candidate = unreal.load_class(None, generated_class_path(asset_path))
        if candidate is not None:
            demo_class = candidate
            source_path = asset_path
            break

    if demo_class is None:
        raise RuntimeError(
            "No generated demo Blueprint was found. "
            "Run install_gravityshift_blueprints.py first."
        )

    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actor = actor_subsystem.spawn_actor_from_class(
        demo_class,
        unreal.Vector(0.0, 0.0, 0.0),
        unreal.Rotator(0.0, 0.0, 0.0),
    )
    if actor is None:
        raise RuntimeError("Could not spawn the Gravity Shift demo room.")

    actor.set_actor_label("GravityShift_DemoRoom")
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.log(f"[GravityShiftDemo] Spawned from {source_path}. Press G in PIE.")


if __name__ == "__main__":
    main()
