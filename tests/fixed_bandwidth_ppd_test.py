#!/usr/bin/env python3
"""Characterize generated PPD support for fixed-bandwidth printers."""

from pathlib import Path
import sys


FIXED_MODELS = (
    "ml1510",
    "ml1520",
    "ml1610",
    "ml1630",
    "ml1640",
    "ml1710",
    "ml1740",
    "ml2240",
    "ml2250",
    "ml2251",
    "ml2510",
    "ph3117",
)

VARIABLE_MODELS = ("ml1750", "ml2010", "ph3122")
ATTRIBUTE = "*QPDL FixedBandWidth: True"


def read_ppd(directory: Path, model: str) -> str:
    path = directory / f"{model}.ppd"
    if not path.is_file():
        raise AssertionError(f"missing generated PPD: {path}")
    return path.read_text(encoding="latin-1")


def main() -> int:
    directory = Path(sys.argv[1] if len(sys.argv) > 1 else "ppd")

    for model in FIXED_MODELS:
        if ATTRIBUTE not in read_ppd(directory, model):
            raise AssertionError(f"{model}.ppd lost fixed-bandwidth support")

    for model in VARIABLE_MODELS:
        if ATTRIBUTE in read_ppd(directory, model):
            raise AssertionError(f"{model}.ppd was incorrectly made fixed-width")

    print("fixed-bandwidth PPD characterization passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
