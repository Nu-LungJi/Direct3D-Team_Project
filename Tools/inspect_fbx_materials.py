import bpy
import sys


fbx = sys.argv[sys.argv.index("--") + 1]
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=fbx)

for obj in bpy.context.scene.objects:
    if obj.type != "MESH":
        continue
    slots = [
        (index, slot.material.name if slot.material else None)
        for index, slot in enumerate(obj.material_slots)
    ]
    used = sorted({poly.material_index for poly in obj.data.polygons})
    print(f"OBJECT={obj.name}")
    print(f"SLOTS={slots!r}")
    print(f"USED={used!r}")
