import bpy
import bmesh
import json
import shutil
from pathlib import Path

import numpy as np


PACKAGE = Path(
    r"C:\Users\gnffk\OneDrive\문서\바탕 화면\새 폴더 (2)\sss"
)
MODEL_DIR = PACKAGE / "Models" / "ProfessorSharp"
TEXTURE_DIR = PACKAGE / "Textures" / "ProfessorSharp"
FBX_PATH = MODEL_DIR / "SK_ProfessorSharp_ProjectReady.fbx"
MANIFEST_PATH = PACKAGE / "ProfessorSharp_ProjectReady_Readme_KR.txt"
MAPPING_PATH = PACKAGE / "ProfessorSharp_Material_Mapping.json"

ROOT = Path(r"D:\Asset\Game")
SOURCE = {
    "face_base": ROOT / "RiggedObjects/Characters/Human/OneOff/Professor_AesopSharp/Textures/T_Adult_M_Head_AesopSharp_BaseColor.png",
    "face_normal": ROOT / "RiggedObjects/Characters/Human/OneOff/Professor_AesopSharp/Textures/T_Adult_M_Head_AesopSharp_Normal.png",
    "jacket_base": ROOT / "RiggedObjects/Characters/Human/OneOff/Professor_AesopSharp/Textures/T_SharpsJacket_BaseColor.png",
    "jacket_normal": ROOT / "RiggedObjects/Characters/Human/OneOff/Professor_AesopSharp/Textures/T_SharpsJacket_Normal.png",
    "jacket_mros": ROOT / "RiggedObjects/Characters/Human/OneOff/Professor_AesopSharp/Textures/T_SharpsJacket_MROS.png",
    "hair_dao": ROOT / "RiggedObjects/Characters/Human/Hair/Hair_M/PROHair04/Textures/T_HUM_M_Hair_PROHair04_AesopLong_DAO_MSK.png",
    "hair_thv": ROOT / "RiggedObjects/Characters/Human/Hair/Hair_M/PROHair04/Textures/T_HUM_M_Hair_PROHair04_AesopLong_THV_MSK.png",
    "scruff_dao": ROOT / "RiggedObjects/Characters/Human/Facial_Hair/Adult_M/AesopScruff/Textures/T_HUM_M_Hair_AesopScruff_DAO_MSK.png",
    "scruff_thv": ROOT / "RiggedObjects/Characters/Human/Facial_Hair/Adult_M/AesopScruff/Textures/T_HUM_M_Hair_AesopScruff_THV_MSK.png",
    "eye_sclera": ROOT / "RiggedObjects/Characters/Human/Heads/Textures/T_Eye_ScleraB_BaseColor.png",
    "eye_iris": ROOT / "RiggedObjects/MasterMaterials/BaseTexture/EyeShader/T_Eye_Iris_BaseColor.png",
    "eyelash": ROOT / "RiggedObjects/MasterMaterials/BaseTexture/EyeLash/T_Eyelash_A.png",
}


def ensure_sources():
    missing = [str(path) for path in SOURCE.values() if not path.is_file()]
    if missing:
        raise RuntimeError("Missing source textures:\n" + "\n".join(missing))


def load_pixels(path):
    image = bpy.data.images.load(str(path), check_existing=False)
    width, height = image.size
    pixels = np.empty(width * height * 4, dtype=np.float32)
    image.pixels.foreach_get(pixels)
    pixels = pixels.reshape((height, width, 4))
    bpy.data.images.remove(image)
    return pixels


def save_pixels(path, pixels, non_color=False):
    height, width, _ = pixels.shape
    image = bpy.data.images.new(
        path.stem,
        width=width,
        height=height,
        alpha=True,
        float_buffer=False,
    )
    image.colorspace_settings.name = "Non-Color" if non_color else "sRGB"
    image.alpha_mode = "STRAIGHT"
    image.pixels.foreach_set(pixels.astype(np.float32, copy=False).ravel())
    image.filepath_raw = str(path)
    image.file_format = "PNG"
    image.save()
    bpy.data.images.remove(image)


def constant_texture(path, rgb, alpha=1.0, size=16, non_color=False):
    pixels = np.empty((size, size, 4), dtype=np.float32)
    pixels[:, :, :3] = np.asarray(rgb, dtype=np.float32)
    pixels[:, :, 3] = alpha
    save_pixels(path, pixels, non_color=non_color)


def copy_texture(source_key, target_name):
    target = TEXTURE_DIR / target_name
    shutil.copy2(SOURCE[source_key], target)
    return target


def create_strand_albedo(dao_key, thv_key, target_name, dark, light):
    dao = load_pixels(SOURCE[dao_key])
    thv = load_pixels(SOURCE[thv_key])
    if dao.shape[:2] != thv.shape[:2]:
        raise RuntimeError(f"Packed hair textures differ in size: {dao_key}")

    variation = np.clip(thv[:, :, 2:3], 0.0, 1.0)
    dark_color = np.asarray(dark, dtype=np.float32).reshape((1, 1, 3))
    light_color = np.asarray(light, dtype=np.float32).reshape((1, 1, 3))
    color = dark_color * (1.0 - variation) + light_color * variation

    output = np.empty_like(dao)
    output[:, :, :3] = color
    # Channel inspection: DAO.G is the real strand opacity.
    output[:, :, 3] = np.clip(dao[:, :, 1], 0.0, 1.0)
    target = TEXTURE_DIR / target_name
    save_pixels(target, output)
    return target


def create_eyelash_albedo(target_name):
    mask = load_pixels(SOURCE["eyelash"])
    luminance = np.clip(mask[:, :, :3].mean(axis=2), 0.0, 1.0)
    output = np.empty_like(mask)
    output[:, :, 0] = 0.012
    output[:, :, 1] = 0.006
    output[:, :, 2] = 0.003
    output[:, :, 3] = luminance
    target = TEXTURE_DIR / target_name
    save_pixels(target, output)
    return target


def create_eye_albedo(target_name, size=1024):
    sclera_source = load_pixels(SOURCE["eye_sclera"])
    iris_source = load_pixels(SOURCE["eye_iris"])
    sh, sw, _ = sclera_source.shape
    ih, iw, _ = iris_source.shape

    y, x = np.mgrid[0:size, 0:size].astype(np.float32)
    u = (x + 0.5) / size
    v = (y + 0.5) / size

    sx = np.clip((u * sw).astype(np.int32), 0, sw - 1)
    sy = np.clip((v * sh).astype(np.int32), 0, sh - 1)
    output = sclera_source[sy, sx].copy()

    radius = np.sqrt((u - 0.5) ** 2 + (v - 0.5) ** 2)
    iris_mask = radius < 0.17
    iris_u = np.clip((u - 0.5) * 3.0 + 0.5, 0.0, 1.0)
    iris_v = np.clip((v - 0.5) * 3.0 + 0.5, 0.0, 1.0)
    ix = np.clip((iris_u * iw).astype(np.int32), 0, iw - 1)
    iy = np.clip((iris_v * ih).astype(np.int32), 0, ih - 1)
    iris = iris_source[iy, ix]
    gray = np.clip(iris[:, :, :3].mean(axis=2), 0.0, 1.0)

    dark = np.asarray((0.001, 0.0005, 0.0002), dtype=np.float32)
    brown = np.asarray((0.24, 0.085, 0.018), dtype=np.float32)
    iris_color = dark + (brown - dark) * gray[:, :, None]
    output[iris_mask, :3] = iris_color[iris_mask]
    output[:, :, 3] = 1.0

    target = TEXTURE_DIR / target_name
    save_pixels(target, output)
    return target


def setup_standard_material(material, albedo_path, normal_path, smro_path, alpha=False):
    material.use_nodes = True
    nodes = material.node_tree.nodes
    links = material.node_tree.links
    nodes.clear()

    output = nodes.new("ShaderNodeOutputMaterial")
    output.location = (700, 0)
    principled = nodes.new("ShaderNodeBsdfPrincipled")
    principled.location = (400, 0)
    links.new(principled.outputs["BSDF"], output.inputs["Surface"])

    albedo = nodes.new("ShaderNodeTexImage")
    albedo.name = "PROJECT_ALBEDO_RGBA"
    albedo.label = "Project Diffuse RGBA"
    albedo.location = (-500, 180)
    albedo.image = bpy.data.images.load(str(albedo_path), check_existing=True)
    links.new(albedo.outputs["Color"], principled.inputs["Base Color"])
    if alpha:
        links.new(albedo.outputs["Alpha"], principled.inputs["Alpha"])
        if hasattr(material, "surface_render_method"):
            material.surface_render_method = "DITHERED"

    normal = nodes.new("ShaderNodeTexImage")
    normal.name = "PROJECT_NORMAL"
    normal.label = "Project Normal"
    normal.location = (-500, -80)
    normal.image = bpy.data.images.load(str(normal_path), check_existing=True)
    normal.image.colorspace_settings.name = "Non-Color"
    normal_map = nodes.new("ShaderNodeNormalMap")
    normal_map.location = (-100, -80)
    links.new(normal.outputs["Color"], normal_map.inputs["Color"])
    links.new(normal_map.outputs["Normal"], principled.inputs["Normal"])

    smro = nodes.new("ShaderNodeTexImage")
    smro.name = "PROJECT_SMRO"
    smro.label = "Project SMRO (R=M, G=Roughness, B=AO)"
    smro.location = (-500, -350)
    smro.image = bpy.data.images.load(str(smro_path), check_existing=True)
    smro.image.colorspace_settings.name = "Non-Color"
    separate = nodes.new("ShaderNodeSeparateColor")
    separate.location = (-100, -350)
    links.new(smro.outputs["Color"], separate.inputs["Color"])
    links.new(separate.outputs["Red"], principled.inputs["Metallic"])
    links.new(separate.outputs["Green"], principled.inputs["Roughness"])


def remove_invisible_simulation_faces(body):
    invisible_indices = {
        index
        for index, slot in enumerate(body.material_slots)
        if slot.material
        and slot.material.name in {"MI_ClothSim", "MI_HUM_N_Heads_EyeOcclusion"}
    }
    if not invisible_indices:
        return 0
    count = sum(
        1 for polygon in body.data.polygons
        if polygon.material_index in invisible_indices
    )
    bm = bmesh.new()
    bm.from_mesh(body.data)
    bmesh.ops.delete(
        bm,
        geom=[
            face for face in bm.faces
            if face.material_index in invisible_indices
        ],
        context="FACES",
    )
    bm.to_mesh(body.data)
    bm.free()
    body.data.update()
    return count


MODEL_DIR.mkdir(parents=True, exist_ok=True)
TEXTURE_DIR.mkdir(parents=True, exist_ok=True)
ensure_sources()

rig = bpy.data.objects.get("ProfessorSharp_MasterRig")
body = bpy.data.objects.get("ProfessorSharp_Body")
hair = bpy.data.objects.get("ProfessorSharp_AesopLong_DynamicHair")
if not rig or not body or not hair:
    raise RuntimeError("Complete professor objects were not found")

removed_sim_faces = remove_invisible_simulation_faces(body)

default_normal = TEXTURE_DIR / "T_Default_Normal.png"
constant_texture(default_normal, (0.5, 0.5, 1.0), non_color=True)

face_albedo = copy_texture("face_base", "T_ProfessorSharp_Face_Albedo.png")
face_normal = copy_texture("face_normal", "T_ProfessorSharp_Face_Normal.png")
face_smro = TEXTURE_DIR / "T_ProfessorSharp_Face_SMRO.png"
constant_texture(face_smro, (0.0, 0.56, 1.0), non_color=True)

jacket_albedo = copy_texture("jacket_base", "T_ProfessorSharp_Clothes_Albedo.png")
jacket_normal = copy_texture("jacket_normal", "T_ProfessorSharp_Clothes_Normal.png")
jacket_smro = copy_texture("jacket_mros", "T_ProfessorSharp_Clothes_SMRO.png")

hair_albedo = create_strand_albedo(
    "hair_dao",
    "hair_thv",
    "T_ProfessorSharp_Hair_AlbedoAlpha.png",
    (0.004, 0.002, 0.001),
    (0.078, 0.035, 0.009),
)
hair_smro = TEXTURE_DIR / "T_ProfessorSharp_Hair_SMRO.png"
constant_texture(hair_smro, (0.0, 0.32, 1.0), non_color=True)

scruff_albedo = create_strand_albedo(
    "scruff_dao",
    "scruff_thv",
    "T_ProfessorSharp_Scruff_AlbedoAlpha.png",
    (0.004, 0.002, 0.001),
    (0.078, 0.035, 0.009),
)
scruff_smro = TEXTURE_DIR / "T_ProfessorSharp_Scruff_SMRO.png"
constant_texture(scruff_smro, (0.0, 0.38, 1.0), non_color=True)

eye_albedo = create_eye_albedo("T_ProfessorSharp_Eye_Albedo.png")
eye_smro = TEXTURE_DIR / "T_ProfessorSharp_Eye_SMRO.png"
constant_texture(eye_smro, (0.0, 0.18, 1.0), non_color=True)

eyelash_albedo = create_eyelash_albedo(
    "T_ProfessorSharp_Eyelash_AlbedoAlpha.png"
)
eyelash_smro = TEXTURE_DIR / "T_ProfessorSharp_Eyelash_SMRO.png"
constant_texture(eyelash_smro, (0.0, 0.45, 1.0), non_color=True)

teeth_albedo = TEXTURE_DIR / "T_ProfessorSharp_Teeth_Albedo.png"
constant_texture(teeth_albedo, (0.72, 0.64, 0.51))
teeth_smro = TEXTURE_DIR / "T_ProfessorSharp_Teeth_SMRO.png"
constant_texture(teeth_smro, (0.0, 0.3, 1.0), non_color=True)

material_map = {
    "MI_Adult_M_Head_AesopSharp": {
        "albedo": face_albedo,
        "normal": face_normal,
        "smro": face_smro,
        "alpha": False,
    },
    "MI_ProfSharp_Clothes": {
        "albedo": jacket_albedo,
        "normal": jacket_normal,
        "smro": jacket_smro,
        "alpha": False,
    },
    "MI_HUM_M_Hair_PROHair04_AesopLong": {
        "albedo": hair_albedo,
        "normal": default_normal,
        "smro": hair_smro,
        "alpha": True,
    },
    "MI_HUM_M_Facial_AesopScruff": {
        "albedo": scruff_albedo,
        "normal": default_normal,
        "smro": scruff_smro,
        "alpha": True,
    },
    "MI_HUM_N_Heads_Eye": {
        "albedo": eye_albedo,
        "normal": default_normal,
        "smro": eye_smro,
        "alpha": False,
    },
    "MI_HUM_N_Heads_Eyelash": {
        "albedo": eyelash_albedo,
        "normal": default_normal,
        "smro": eyelash_smro,
        "alpha": True,
    },
    "MI_HUM_N_Heads_Teeth_Decay3": {
        "albedo": teeth_albedo,
        "normal": default_normal,
        "smro": teeth_smro,
        "alpha": False,
    },
}

for material_name, entry in material_map.items():
    material = bpy.data.materials.get(material_name)
    if material:
        setup_standard_material(
            material,
            entry["albedo"],
            entry["normal"],
            entry["smro"],
            alpha=entry["alpha"],
        )

for obj in bpy.data.objects:
    obj.animation_data_clear()
for action in list(bpy.data.actions):
    bpy.data.actions.remove(action, do_unlink=True)

bpy.ops.object.select_all(action="DESELECT")
for obj in (rig, body, hair):
    obj.hide_set(False)
    obj.hide_viewport = False
    obj.select_set(True)
bpy.context.view_layer.objects.active = rig

bpy.ops.export_scene.fbx(
    filepath=str(FBX_PATH),
    use_selection=True,
    object_types={"ARMATURE", "MESH"},
    apply_unit_scale=True,
    apply_scale_options="FBX_SCALE_ALL",
    use_space_transform=True,
    bake_space_transform=False,
    add_leaf_bones=False,
    use_armature_deform_only=False,
    armature_nodetype="NULL",
    bake_anim=False,
    path_mode="ABSOLUTE",
    embed_textures=False,
    mesh_smooth_type="FACE",
    use_mesh_modifiers=True,
)

mapping_json = {
    material_name: {
        "DiffuseRGBA": str(entry["albedo"]),
        "Normal": str(entry["normal"]),
        "SMRO": str(entry["smro"]),
        "UsesAlphaCutout": entry["alpha"],
    }
    for material_name, entry in material_map.items()
}
MAPPING_PATH.write_text(
    json.dumps(mapping_json, ensure_ascii=False, indent=2),
    encoding="utf-8-sig",
)

MANIFEST_PATH.write_text(
    "Professor Sharp 프로젝트용 패키지\n\n"
    f"모델: {FBX_PATH}\n"
    f"텍스처: {TEXTURE_DIR}\n"
    "애니메이션: 없음\n"
    f"전체 본: {len(rig.data.bones)}개\n"
    "헤어 본: 15개\n"
    f"제거한 보조 시뮬레이션 면: {removed_sim_faces}개\n\n"
    "엔진 슬롯 규칙\n"
    "Diffuse: Albedo RGBA (헤어/수염/속눈썹 가닥은 Alpha)\n"
    "Normal: Tangent-space Normal\n"
    "SMRO: R=Metallic, G=Roughness, B=AO\n\n"
    "권장 셰이더 알파 컷\n"
    "clip(fDiffuse.a - 0.3f);\n\n"
    "Blender 전용 노드 없이 표준 Principled 연결만 사용해 FBX를 내보냈습니다.\n",
    encoding="utf-8-sig",
)

print("PACKAGE", PACKAGE)
print("FBX", FBX_PATH, FBX_PATH.stat().st_size)
print("TEXTURES", len(list(TEXTURE_DIR.glob("*.png"))))
print("BONES", len(rig.data.bones))
print("ACTIONS", len(bpy.data.actions))
