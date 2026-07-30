import bpy
from pathlib import Path
import numpy as np


PACKAGE = Path(
    r"C:\Users\gnffk\OneDrive\문서\바탕 화면\새 폴더 (2)\sss"
)
FBX = PACKAGE / "Models/ProfessorSharp/SK_ProfessorSharp_ProjectReady.fbx"
TEXTURES = PACKAGE / "Textures/ProfessorSharp"

bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)
bpy.ops.import_scene.fbx(filepath=str(FBX), use_anim=True)

armatures = [obj for obj in bpy.data.objects if obj.type == "ARMATURE"]
meshes = [obj for obj in bpy.data.objects if obj.type == "MESH"]
image_paths = {
    Path(image.filepath).resolve()
    for image in bpy.data.images
    if image.source == "FILE" and image.filepath
}
missing_image_files = [str(path) for path in image_paths if not path.is_file()]

hair_image = bpy.data.images.load(
    str(TEXTURES / "T_ProfessorSharp_Hair_AlbedoAlpha.png"),
    check_existing=True,
)
scruff_image = bpy.data.images.load(
    str(TEXTURES / "T_ProfessorSharp_Scruff_AlbedoAlpha.png"),
    check_existing=True,
)

def alpha_range(image):
    pixels = np.empty(len(image.pixels), dtype=np.float32)
    image.pixels.foreach_get(pixels)
    alpha = pixels[3::4]
    return float(alpha.min()), float(alpha.max())

hair_alpha = alpha_range(hair_image)
scruff_alpha = alpha_range(scruff_image)
all_png = sorted(TEXTURES.glob("*.png"))

print("FBX_EXISTS", FBX.is_file(), FBX.stat().st_size)
print("ARMATURES", [(obj.name, len(obj.data.bones)) for obj in armatures])
print("MESHES", [(obj.name, len(obj.data.polygons)) for obj in meshes])
print("MATERIALS", sorted(material.name for material in bpy.data.materials))
print("IMAGES_REFERENCED", len(image_paths))
print("IMAGE_PATHS", sorted(str(path) for path in image_paths))
print("MISSING_IMAGES", missing_image_files)
print("PNG_COUNT", len(all_png))
print("ACTIONS", len(bpy.data.actions))
print("HAIR_ALPHA_RANGE", hair_alpha)
print("SCRUFF_ALPHA_RANGE", scruff_alpha)

if len(armatures) != 1 or len(armatures[0].data.bones) != 230:
    raise RuntimeError("Expected one 230-bone armature")
if len(meshes) < 2:
    raise RuntimeError("Expected body and dynamic-hair meshes")
if missing_image_files:
    raise RuntimeError("FBX references missing texture files")
if len(all_png) != 17:
    raise RuntimeError(f"Expected 17 textures, got {len(all_png)}")
if bpy.data.actions:
    raise RuntimeError("Unexpected animations were exported")
if not (hair_alpha[0] < 0.05 and hair_alpha[1] > 0.95):
    raise RuntimeError("Hair alpha cutout was not baked")
if not (scruff_alpha[0] < 0.05 and scruff_alpha[1] > 0.95):
    raise RuntimeError("Scruff alpha cutout was not baked")
