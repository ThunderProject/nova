#!/usr/bin/env python3

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from dataclasses import dataclass
from enum import Enum
from pathlib import Path


class Config(Enum):
    DEBUG = "Debug"
    RELEASE = "Release"

    @property
    def as_str(self) -> str:
        return self.value


@dataclass(frozen=True)
class PackageMatch:
    name: str
    path: Path


def normalize_name(value: str) -> str:
    return re.sub(r"[^a-z0-9]+", "", value.casefold())


def ensure_workspace_root() -> None:
    required = ["conanfile.py", "CMakeLists.txt"]
    for item in required:
        if not Path(item).exists():
            raise RuntimeError(
                f"run this from the packages/ workspace root; missing '{item}'"
            )


def run_command(program: str, args: list[str]) -> None:
    print(f"> {program} {' '.join(args)}")

    try:
        completed = subprocess.run([program, *args], check=False)
    except OSError as exc:
        raise RuntimeError(f"failed to run '{program}': {exc}") from exc

    if completed.returncode != 0:
        raise RuntimeError(
            f"command failed with status {completed.returncode}: "
            f"{program} {' '.join(args)}"
        )


def capture_command(program: str, args: list[str]) -> str:
    print(f"> {program} {' '.join(args)}")

    try:
        completed = subprocess.run(
            [program, *args],
            check=False,
            text=True,
            capture_output=True,
        )
    except OSError as exc:
        raise RuntimeError(f"failed to run '{program}': {exc}") from exc

    if completed.returncode != 0:
        stderr = completed.stderr.strip()
        raise RuntimeError(
            f"command failed with status {completed.returncode}: "
            f"{program} {' '.join(args)}"
            + (f"\n{stderr}" if stderr else "")
        )

    return completed.stdout


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        prog="Nova Build Tool",
        description="Build tool for Nova packages",
    )

    parser.add_argument(
        "--debug",
        action="store_true",
        help="build Debug instead of Release",
    )
    parser.add_argument(
        "--release",
        action="store_true",
        help="explicitly build Release (default)",
    )

    parser.add_argument(
        "--all",
        action="store_true",
        help="build everything (default if no package is given)",
    )
    parser.add_argument(
        "-p",
        "--package",
        type=str,
        help="build a single package by name",
    )
    parser.add_argument(
        "--tests",
        action="store_true",
        help="run tests after build",
    )
    parser.add_argument(
        "--fetch",
        action="store_true",
        help="run conan install before configuring",
    )

    cli = parser.parse_args()

    if cli.debug and cli.release:
        raise RuntimeError("choose at most one of --debug or --release")

    if cli.all and cli.package:
        raise RuntimeError("choose either --all or --package, not both")

    return cli


def discover_package_dirs(root: Path) -> list[PackageMatch]:
    matches: list[PackageMatch] = []

    skip_dirs = {
        ".git",
        ".idea",
        ".vscode",
        ".zed",
        "build",
        "out",
        ".cache",
        "__pycache__",
    }

    for cmakelists in root.rglob("CMakeLists.txt"):
        parent = cmakelists.parent

        if parent == root:
            continue

        if any(part in skip_dirs for part in parent.parts):
            continue

        matches.append(PackageMatch(name=parent.name, path=parent))

    dedup: dict[Path, PackageMatch] = {}
    for match in matches:
        dedup[match.path] = match

    return sorted(dedup.values(), key=lambda item: str(item.path))


def resolve_package_dir(root: Path, query: str) -> Path:
    query_norm = normalize_name(query)
    matches: list[Path] = []

    for cmakelists in root.rglob("CMakeLists.txt"):
        parent = cmakelists.parent
        if parent == root:
            continue
        if normalize_name(parent.name) == query_norm:
            matches.append(parent)

    if not matches:
        raise RuntimeError(f"unknown package '{query}'")

    if len(matches) > 1:
        raise RuntimeError(
            f"package name '{query}' is ambiguous: "
            + ", ".join(str(p) for p in matches)
        )

    return matches[0]


def extract_targets_from_cmakelists(cmakelists: Path) -> list[str]:
    text = cmakelists.read_text(encoding="utf-8")

    patterns = [
        r"add_library\s*\(\s*([A-Za-z0-9_:\-\.]+)",
        r"add_executable\s*\(\s*([A-Za-z0-9_:\-\.]+)",
    ]

    targets: list[str] = []
    seen: set[str] = set()

    for pattern in patterns:
        for match in re.finditer(pattern, text, flags=re.IGNORECASE):
            target = match.group(1)
            if target.upper() in {"STATIC", "SHARED", "MODULE", "OBJECT", "INTERFACE", "ALIAS"}:
                continue
            if target not in seen:
                seen.add(target)
                targets.append(target)

    return targets


def resolve_target_for_package(package_dir: Path, query: str) -> str:
    targets = extract_targets_from_cmakelists(package_dir / "CMakeLists.txt")

    if not targets:
        raise RuntimeError(f"no targets found in {package_dir / 'CMakeLists.txt'}")

    if len(targets) == 1:
        return targets[0]

    package_norm = normalize_name(package_dir.name)
    matches = [t for t in targets if package_norm in normalize_name(t)]

    if len(matches) == 1:
        return matches[0]

    raise RuntimeError(
        f"multiple targets found in {package_dir / 'CMakeLists.txt'}: {', '.join(targets)}"
    )

def list_build_targets(build_dir: str) -> list[str]:
    output = capture_command("cmake", ["--build", build_dir, "--target", "help"])

    targets: set[str] = set()
    for line in output.splitlines():
        stripped = line.strip()

        if not stripped:
            continue

        if stripped.startswith("... "):
            stripped = stripped[4:]

        stripped = stripped.removesuffix(" (the default if no target is provided)")
        stripped = stripped.strip()

        if not stripped:
            continue

        if "..." in stripped:
            continue

        if any(ch.isspace() for ch in stripped):
            continue

        targets.add(stripped)

    return sorted(targets)

def maybe_run_conan_fetch(config: Config) -> None:
    run_command(
        "conan",
        [
            "install",
            ".",
            "--build=missing",
            "-s",
            f"build_type={config.as_str}",
        ],
    )


def configure_cmake(config: Config, fetch: bool) -> str:
    cmake_build_dir = f"build/{config.as_str}/cmake"
    toolchain_file = Path(f"build/{config.as_str}/generators/conan_toolchain.cmake")

    args = [
        "-S",
        ".",
        "-B",
        cmake_build_dir,
        "-G",
        "Ninja",
        f"-DCMAKE_BUILD_TYPE={config.as_str}",
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
    ]

    if toolchain_file.exists():
        args.append(f"-DCMAKE_TOOLCHAIN_FILE={toolchain_file}")
    elif fetch:
        raise RuntimeError(
            f"expected Conan toolchain at '{toolchain_file}', but it was not generated"
        )
    else:
        print(
            f"note: no Conan toolchain found at '{toolchain_file}', "
            "configuring without it"
        )

    run_command("cmake", args)
    return cmake_build_dir


def run() -> None:
    ensure_workspace_root()

    cli = parse_args()
    config = Config.DEBUG if cli.debug else Config.RELEASE

    if cli.fetch:
        maybe_run_conan_fetch(config)

    cmake_build_dir = configure_cmake(config, cli.fetch)

    if cli.package:
        package_dir = resolve_package_dir(Path.cwd(), cli.package)
        target = resolve_target_for_package(package_dir, cli.package)
        print(f"resolved package '{cli.package}' -> {package_dir}")
        print(f"resolved target '{target}'")
        run_command("cmake", ["--build", cmake_build_dir, "--target", target])
    else:
        run_command("cmake", ["--build", cmake_build_dir])

    if cli.tests:
        run_command("ctest", ["--test-dir", cmake_build_dir, "--output-on-failure"])


def main() -> None:
    try:
        run()
    except RuntimeError as error:
        print(f"error: {error}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
