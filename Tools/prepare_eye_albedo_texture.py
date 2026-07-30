from pathlib import Path
import sys

from PIL import Image


generated_path = Path(sys.argv[1])
original_path = Path(sys.argv[2])
output_path = Path(sys.argv[3])

generated = Image.open(generated_path).convert("RGB")
original = Image.open(original_path).convert("RGBA")

if generated.size != original.size:
    generated = generated.resize(original.size, Image.Resampling.LANCZOS)

result = Image.merge(
    "RGBA",
    (
        generated.getchannel("R"),
        generated.getchannel("G"),
        generated.getchannel("B"),
        original.getchannel("A"),
    ),
)
output_path.parent.mkdir(parents=True, exist_ok=True)
result.save(output_path, "PNG")
print(f"OUTPUT={output_path}")
print(f"SIZE={result.size}")
print(f"MODE={result.mode}")
