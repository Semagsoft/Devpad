#!/usr/bin/env python3
"""Convert SVG to .ico (multi-size Windows icon)."""
import struct, zlib, subprocess, sys, os, tempfile

def create_ico(svg_path, ico_path, sizes=(16, 32, 48, 64, 128, 256)):
    pngs = []
    rsvg = os.environ.get("RSVG_CONVERT", "rsvg-convert")
    for s in sizes:
        with tempfile.NamedTemporaryFile(suffix=".png", delete=False) as f:
            tmp = f.name
        subprocess.run([rsvg, "-w", str(s), "-h", str(s), svg_path, "-o", tmp], check=True)
        with open(tmp, "rb") as f:
            pngs.append(f.read())
        os.unlink(tmp)

    count = len(pngs)
    header = struct.pack("<HHH", 0, 1, count)
    entries = []
    offset = 6 + 16 * count
    for i, png in enumerate(pngs):
        w = sizes[i] if sizes[i] < 256 else 0
        h = sizes[i] if sizes[i] < 256 else 0
        entries.append(struct.pack("<BBBBHHII", w, h, 0, 0, 1, 32, len(png), offset))
        offset += len(png)

    with open(ico_path, "wb") as f:
        f.write(header)
        for e in entries:
            f.write(e)
        for png in pngs:
            f.write(png)

if __name__ == "__main__":
    script_dir = os.path.dirname(os.path.abspath(__file__))
    repo_dir = os.path.dirname(script_dir)
    svg = sys.argv[1] if len(sys.argv) > 1 else os.path.join(repo_dir, "resources/icons/devpad.svg")
    ico = sys.argv[2] if len(sys.argv) > 2 else os.path.join(script_dir, "devpad.ico")
    create_ico(svg, ico)
    print(f"Created {ico} ({os.path.getsize(ico)} bytes)")
