import argparse
import struct
from pathlib import Path


HEADER_SIZE = 20
CHUNK_MATERIAL = 1
CHUNK_BONE = 3
TEXTURE_METALNESS = 15

SMRO_BY_MATERIAL = {
    0: "T_ProfessorSharp_Eyelash_SMRO.png",
    1: "T_ProfessorSharp_Eye_SMRO.png",
    2: "T_ProfessorSharp_Face_SMRO.png",
    3: "T_ProfessorSharp_Teeth_SMRO.png",
    4: "T_ProfessorSharp_Clothes_SMRO.png",
    5: "T_ProfessorSharp_Scruff_SMRO.png",
    6: "T_ProfessorSharp_Hair_SMRO.png",
}


def u32(data, offset):
    return struct.unpack_from("<I", data, offset)[0], offset + 4


def pack_texture(texture_type, filename):
    path = Path(filename)
    stem = path.stem.encode("utf-8")
    ext = path.suffix.encode("utf-8")
    return (
        struct.pack("<II", texture_type, len(stem))
        + stem
        + struct.pack("<I", len(ext))
        + ext
    )


def rewrite_materials(payload, material_count):
    offset = 0
    output = bytearray()
    for _ in range(material_count):
        record_size, offset = u32(payload, offset)
        record_start = offset
        record_end = record_start + record_size
        material_num, offset = u32(payload, offset)
        group_count, offset = u32(payload, offset)
        groups = payload[offset:record_end]

        smro = SMRO_BY_MATERIAL.get(material_num)
        if smro:
            groups += struct.pack("<I", 1) + pack_texture(TEXTURE_METALNESS, smro)
            group_count += 1

        record = struct.pack("<II", material_num, group_count) + groups
        output += struct.pack("<I", len(record)) + record
        offset = record_end
    if offset != len(payload):
        raise ValueError("Material payload was not consumed exactly")
    return bytes(output)


def chunk_payload(data, wanted_type):
    offset = HEADER_SIZE
    while offset < len(data):
        chunk_type, size = struct.unpack_from("<II", data, offset)
        start = offset + 8
        end = start + size
        if chunk_type == wanted_type:
            return data[start:end]
        offset = end
    raise ValueError(f"Chunk {wanted_type} not found")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("reference")
    parser.add_argument("model")
    parser.add_argument("output")
    args = parser.parse_args()

    reference = Path(args.reference).read_bytes()
    model = Path(args.model).read_bytes()
    ref_bone_count = struct.unpack_from("<I", reference, 16)[0]
    model_bone_count = struct.unpack_from("<I", model, 16)[0]
    if ref_bone_count != model_bone_count:
        raise ValueError(
            f"Bone count mismatch: {ref_bone_count} != {model_bone_count}"
        )

    reference_bones = chunk_payload(reference, CHUNK_BONE)
    material_count = struct.unpack_from("<I", model, 8)[0]

    output = bytearray(model[:HEADER_SIZE])
    offset = HEADER_SIZE
    while offset < len(model):
        chunk_type, size = struct.unpack_from("<II", model, offset)
        start = offset + 8
        end = start + size
        payload = model[start:end]
        if chunk_type == CHUNK_BONE:
            payload = reference_bones
        elif chunk_type == CHUNK_MATERIAL:
            payload = rewrite_materials(payload, material_count)
        output += struct.pack("<II", chunk_type, len(payload)) + payload
        offset = end

    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_bytes(output)
    print(f"OUTPUT={output_path}")
    print(f"BONES={model_bone_count}")
    print(f"MATERIALS={material_count}")


if __name__ == "__main__":
    main()
