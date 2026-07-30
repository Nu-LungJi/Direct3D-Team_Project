import argparse
import struct
from pathlib import Path


HEADER_SIZE = 20
CHUNK_HEADER_SIZE = 8
CHUNK_BONE = 3


def chunks(data: bytes):
    offset = HEADER_SIZE
    while offset < len(data):
        chunk_type, size = struct.unpack_from("<II", data, offset)
        payload_start = offset + CHUNK_HEADER_SIZE
        payload_end = payload_start + size
        if payload_end > len(data):
            raise ValueError(f"Invalid chunk at {offset}: {size} bytes")
        yield offset, chunk_type, payload_start, payload_end
        offset = payload_end


def bone_chunk(data: bytes):
    for offset, chunk_type, payload_start, payload_end in chunks(data):
        if chunk_type == CHUNK_BONE:
            return offset, payload_start, payload_end
    raise ValueError("Bone chunk not found")


def bone_count(data: bytes) -> int:
    return struct.unpack_from("<I", data, 16)[0]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("reference")
    parser.add_argument("model")
    parser.add_argument("output")
    args = parser.parse_args()

    reference = Path(args.reference).read_bytes()
    model = Path(args.model).read_bytes()

    if bone_count(reference) != bone_count(model):
        raise ValueError(
            f"Bone count mismatch: reference={bone_count(reference)}, "
            f"model={bone_count(model)}"
        )

    _, ref_start, ref_end = bone_chunk(reference)
    model_header, model_start, model_end = bone_chunk(model)
    ref_payload = reference[ref_start:ref_end]

    result = bytearray()
    result += model[:model_header]
    result += struct.pack("<II", CHUNK_BONE, len(ref_payload))
    result += ref_payload
    result += model[model_end:]

    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(result)
    print(f"OUTPUT={output}")
    print(f"BONE_COUNT={bone_count(result)}")
    print(f"BONE_CHUNK_BYTES={len(ref_payload)}")


if __name__ == "__main__":
    main()
