import bpy
import os
import sys


args = sys.argv[sys.argv.index("--") + 1:]
output_fbx = os.path.abspath(args[0])

for material in bpy.data.materials:
    if not material.use_nodes or material.node_tree is None:
        continue
    nodes = material.node_tree.nodes
    links = material.node_tree.links
    principled = next((n for n in nodes if n.type == "BSDF_PRINCIPLED"), None)
    smro = next(
        (
            n for n in nodes
            if n.type == "TEX_IMAGE"
            and n.image
            and "_SMRO" in os.path.basename(n.image.filepath)
        ),
        None,
    )
    if principled is None or smro is None:
        continue
    metallic = principled.inputs.get("Metallic")
    for link in list(metallic.links):
        links.remove(link)
    # A direct image link is recognized by Blender's FBX exporter as an
    # Assimp aiTextureType_METALNESS slot. The engine consumes the entire
    # packed image from that slot as R=metallic, G=roughness, B=AO.
    links.new(smro.outputs["Color"], metallic)

body = bpy.data.objects["ProfessorSharp_Body"]
rig = bpy.data.objects["ProfessorSharp_MasterRig"]
bpy.ops.object.select_all(action="DESELECT")
body.select_set(True)
rig.select_set(True)
bpy.context.view_layer.objects.active = rig

os.makedirs(os.path.dirname(output_fbx), exist_ok=True)
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
print(f"SMRO_FBX={output_fbx}")
