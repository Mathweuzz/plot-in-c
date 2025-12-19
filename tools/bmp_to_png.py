#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
from PIL import Image


def main() -> int:
    screenshots = Path("screenshots")
    if not screenshots.exists() or not screenshots.is_dir():
        print("ERRO: pasta ./screenshots não existe.")
        return 1

    bmps = sorted(screenshots.glob("*.bmp"))
    if not bmps:
        print("Nada pra converter: nenhum .bmp em ./screenshots")
        return 0

    converted = 0
    for bmp in bmps:
        png = bmp.with_suffix(".png")
        try:
            with Image.open(bmp) as im:
                # Garante modo compatível com PNG (caso venha com formato estranho)
                im = im.convert("RGBA") if im.mode in ("P", "LA") else im.convert("RGB")
                im.save(png, format="PNG", optimize=True)
            converted += 1
            print(f"OK: {bmp} -> {png}")
        except Exception as e:
            print(f"FALHOU: {bmp} ({e})")

    print(f"\nConcluído: {converted}/{len(bmps)} convertidos.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
