import bpy
import os
import sys


def arg_after_dash(index: int) -> str:
    args = sys.argv
    if "--" not in args:
        raise RuntimeError("Expected arguments after --")
    return args[args.index("--") + 1 + index]


output_blend = os.path.abspath(arg_after_dash(0))
output_fbx = os.path.abspath(arg_after_dash(1))

rig = bpy.data.objects.get("ProfessorSharp_MasterRig")
body = bpy.data.objects.get("ProfessorSharp_Body")
hair = bpy.data.objects.get("ProfessorSharp_AesopLong_DynamicHair")

if rig is None or rig.type != "ARMATURE":
    raise RuntimeError("ProfessorSharp_MasterRig armature not found")
if body is None or body.type != "MESH":
    raise RuntimeError("ProfessorSharp_Body mesh not found")
if hair is None or hair.type != "MESH":
    raise RuntimeError("ProfessorSharp_AesopLong_DynamicHair mesh not found")
if rig.data.bones.get("Head") is None:
    raise RuntimeError("Head bone not found")

# The existing professor skeleton has no dynamic hair bones. Bind all hair
# vertices to Head so the hair follows every existing professor animation.
for group in list(hair.vertex_groups):
    hair.vertex_groups.remove(group)
head_group = hair.vertex_groups.new(name="Head")
head_group.add(range(len(hair.data.vertices)), 1.0, "REPLACE")

for modifier in list(hair.modifiers):
    if modifier.type == "ARMATURE":
        modifier.object = rig

# Joining removes the extra hair object node that otherwise becomes an
# additional "bone" in the custom Assimp conversion pipeline.
bpy.ops.object.mode_set(mode="OBJECT") if bpy.context.object and bpy.context.object.mode != "OBJECT" else None
bpy.ops.object.select_all(action="DESELECT")
body.select_set(True)
hair.select_set(True)
bpy.context.view_layer.objects.active = body
bpy.ops.object.join()

# Remove only the generated dynamic hair bones. The original 218-node
# professor hierarchy remains untouched.
dynamic_hair_bones = {
    "L_hair_00", "L_hair_01", "L_hair_02", "L_hair_03", "L_hair_04",
    "L_hair_05", "L_hair_06", "L_hair_07", "L_hair_08",
    "R_hair_00", "R_hair_01", "R_hair_02", "R_hair_03", "R_hair_04",
    "R_hair_05",
}
bpy.context.view_layer.objects.active = rig
rig.select_set(True)
bpy.ops.object.mode_set(mode="EDIT")
for bone in list(rig.data.edit_bones):
    if bone.name in dynamic_hair_bones:
        rig.data.edit_bones.remove(bone)
bpy.ops.object.mode_set(mode="OBJECT")

remaining = sorted(b.name for b in rig.data.bones if b.name in dynamic_hair_bones)
if remaining:
    raise RuntimeError(f"Dynamic hair bones still remain: {remaining}")

os.makedirs(os.path.dirname(output_blend), exist_ok=True)
os.makedirs(os.path.dirname(output_fbx), exist_ok=True)
bpy.ops.wm.save_as_mainfile(filepath=output_blend)

bpy.ops.object.select_all(action="DESELECT")
body.select_set(True)
rig.select_set(True)
bpy.context.view_layer.objects.active = rig

bpy.ops.export_scene.fbx(
    filepath=output_fbx,
    use_selection=True,
    object_types={"ARMATURE", "MESH"},
    use_mesh_modifiers=True,
    add_leaf_bones=False,
    bake_anim=False,
    path_mode="ABSOLUTE",
    embed_textures=False,
    axis_forward="-Z",
    axis_up="Y",
)

print(f"COMPAT_BLEND={output_blend}")
print(f"COMPAT_FBX={output_fbx}")
print(f"ARMATURE_BONES={len(rig.data.bones)}")
