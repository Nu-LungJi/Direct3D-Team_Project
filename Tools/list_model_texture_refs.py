import argparse
import json
import struct
from pathlib import Path


HEADER_SIZE = 20
CHUNK_MATERIAL = 1


def read_u32(data: bytes, offset: int):
    return struct.unpack_from("<I", data, offset)[0], offset + 4


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("model")
    parser.add_argument("--texture-dir")
    args = parser.parse_args()

    data = Path(args.model).read_bytes()
    material_count = struct.unpack_from("<I", data, 8)[0]
    offset = HEADER_SIZE
    references = []

    while offset < len(data):
        chunk_type, chunk_size = struct.unpack_from("<II", data, offset)
        offset += 8
        chunk_end = offset + chunk_size
        if chunk_end > len(data):
            raise ValueError("Invalid chunk size")

        if chunk_type == CHUNK_MATERIAL:
            for _ in range(material_count):
                record_size, offset = read_u32(data, offset)
                record_end = offset + record_size
                material_num, offset = read_u32(data, offset)
                group_count, offset = read_u32(data, offset)
                for _ in range(group_count):
                    texture_count, offset = read_u32(data, offset)
                    for _ in range(texture_count):
                        texture_type, offset = read_u32(data, offset)
                        file_len, offset = read_u32(data, offset)
                        file_name = data[offset:offset + file_len].decode("utf-8")
                        offset += file_len
                        ext_len, offset = read_u32(data, offset)
                        extension = data[offset:offset + ext_len].decode("utf-8")
                        offset += ext_len
                        references.append(
                            {
                                "material": material_num,
                                "type": texture_type,
                                "file": file_name + extension,
                            }
                        )
                offset = record_end
        offset = chunk_end

    texture_dir = Path(args.texture_dir) if args.texture_dir else None
    for ref in references:
        ref["exists"] = (
            (texture_dir / ref["file"]).is_file() if texture_dir else None
        )

    print(json.dumps(references, ensure_ascii=False, indent=2))
    if texture_dir:
        missing = sorted({ref["file"] for ref in references if not ref["exists"]})
        print(f"MISSING_COUNT={len(missing)}")
        for name in missing:
            print(f"MISSING={name}")


if __name__ == "__main__":
    main()
