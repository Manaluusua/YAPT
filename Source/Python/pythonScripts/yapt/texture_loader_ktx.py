from __future__ import annotations

import hashlib
import json
import logging
import os
import shutil
import subprocess
import tempfile
from collections.abc import Callable, Mapping, Sequence
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any
from pyktx.ktx_texture2 import KtxTexture2
from pyktx.ktx_texture_create_flag_bits import KtxTextureCreateFlagBits

logger = logging.getLogger(__name__)

CacheStemFn = Callable[[str, Sequence[Path], Mapping[str, Any]], str]


class KtxToolNotFoundError(FileNotFoundError):
    """Raised when the `ktx` executable is required but not found on PATH."""


class KtxToolInvocationError(RuntimeError):
    """Raised when the KTX CLI exits with a non-zero status."""

    def __init__(
        self,
        message: str,
        *,
        command: Sequence[str],
        returncode: int,
        stderr: str,
    ) -> None:
        super().__init__(message)
        self.command = list(command)
        self.returncode = returncode
        self.stderr = stderr


def default_cache_stem(kind: str, sources: Sequence[Path], options: Mapping[str, Any]) -> str:
    """Build `{kind}_{sha256}.ktx2` stem from sources and conversion options."""

    payload = {
        "kind": kind,
        "sources": [str(p.resolve()) for p in sources],
        "vk_format": options.get("vk_format"),
        "extra_ktx_args": list(options.get("extra_ktx_args") or ()),
        "ktx_version": options.get("ktx_version") or "",
    }
    digest = hashlib.sha256(
        json.dumps(payload, sort_keys=True, ensure_ascii=False).encode()
    ).hexdigest()
    return f"{kind}_{digest}"


@dataclass
class TextureLoader:
    """
    Load KTX/KTX2 textures with pyktx. Convert PNG/EXR sources via the Khronos
    ``ktx create`` CLI, caching outputs under ``cache_dir``.

    Cubemap inputs must be ordered +X, -X, +Y, -Y, +Z, -Z (``ktx create --cubemap``).
    """

    cache_dir: Path | str = field(default_factory=lambda: Path(".ktx_cache"))
    ktx_executable: str = "ktx"
    default_vk_format: str = "R8G8B8A8_SRGB"
    cache_stem_fn: CacheStemFn | None = None

    def __post_init__(self) -> None:
        self._cache_dir = Path(self.cache_dir)
        self._ktx_version: str | None = None

    def load_ktx_texture(
        self,
        path: Path | str,
        *,
        create_flags: int | None = None,
    ) -> Any:
        """Load an existing KTX file using pyktx."""

        KtxTexture2 = _import_ktx_texture2()
        KtxTextureCreateFlagBits = _import_ktx_create_flags()
        resolved = Path(path).expanduser().resolve()
        flags = (
            create_flags
            if create_flags is not None
            else KtxTextureCreateFlagBits.LOAD_IMAGE_DATA_BIT
        )
        return KtxTexture2.create_from_named_file(str(resolved), create_flags=flags)

    def load_texture_2d(
        self,
        source: Path | str,
        *,
        vk_format: str | None = None,
        extra_ktx_args: Sequence[str] = (),
        force_reconvert: bool = False,
        cache_stem_fn: CacheStemFn | None = None,
    ) -> Any:
        """Convert a single image to KTX2 (if needed), then load with pyktx."""

        src = Path(source).expanduser().resolve()
        if not src.is_file():
            raise FileNotFoundError(f"Source texture not found: {src}")

        fmt = vk_format or self.default_vk_format
        extra = tuple(extra_ktx_args)
        out = self._cache_path(
            "2d",
            (src,),
            vk_format=fmt,
            extra_ktx_args=extra,
            stem_fn=cache_stem_fn,
        )

        if force_reconvert and out.exists():
            out.unlink()

        if not out.exists():
            logger.info("KTX cache miss, converting to %s", out)
            self._convert_with_cli(out, lambda tmp: self._build_create_command_2d(src, tmp, fmt, extra))
        else:
            logger.info("KTX cache hit: %s", out)

        return self.load_ktx_texture(out)

    def load_texture_cube(
        self,
        faces: Sequence[Path | str],
        *,
        vk_format: str | None = None,
        extra_ktx_args: Sequence[str] = (),
        force_reconvert: bool = False,
        cache_stem_fn: CacheStemFn | None = None,
    ) -> Any:
        """
        Convert six face images to a cubemap KTX2 (if needed), then load.

        ``faces`` must be in ``ktx create`` order: +X, -X, +Y, -Y, +Z, -Z.
        """

        if len(faces) != 6:
            raise ValueError(f"Cubemap requires 6 face images, got {len(faces)}")

        resolved = [Path(p).expanduser().resolve() for p in faces]
        for p in resolved:
            if not p.is_file():
                raise FileNotFoundError(f"Cubemap face not found: {p}")

        fmt = vk_format or self.default_vk_format
        extra = tuple(extra_ktx_args)
        out = self._cache_path(
            "cube",
            tuple(resolved),
            vk_format=fmt,
            extra_ktx_args=extra,
            stem_fn=cache_stem_fn,
        )

        if force_reconvert and out.exists():
            out.unlink()

        if not out.exists():
            logger.info("KTX cache miss, converting to %s", out)
            self._convert_with_cli(
                out,
                lambda tmp: self._build_create_command_cube(resolved, tmp, fmt, extra),
            )
        else:
            logger.info("KTX cache hit: %s", out)

        return self.load_ktx_texture(out)

    def load_texture_3d(
        self,
        slices: Sequence[Path | str],
        *,
        vk_format: str | None = None,
        extra_ktx_args: Sequence[str] = (),
        force_reconvert: bool = False,
        cache_stem_fn: CacheStemFn | None = None,
    ) -> Any:
        """
        Convert ordered slice images to a 3D KTX2 (if needed), then load.

        ``slices`` are stacked along Z; ``ktx create`` receives ``--depth`` equal to
        the slice count and one input file per slice, in order.
        """

        if len(slices) < 1:
            raise ValueError("3D texture requires at least one slice image")

        resolved = [Path(p).expanduser().resolve() for p in slices]
        for p in resolved:
            if not p.is_file():
                raise FileNotFoundError(f"3D slice not found: {p}")

        fmt = vk_format or self.default_vk_format
        extra = tuple(extra_ktx_args)
        out = self._cache_path(
            "3d",
            tuple(resolved),
            vk_format=fmt,
            extra_ktx_args=extra,
            stem_fn=cache_stem_fn,
        )

        if force_reconvert and out.exists():
            out.unlink()

        if not out.exists():
            logger.info("KTX cache miss, converting to %s", out)
            self._convert_with_cli(
                out,
                lambda tmp: self._build_create_command_3d(resolved, tmp, fmt, extra),
            )
        else:
            logger.info("KTX cache hit: %s", out)

        return self.load_ktx_texture(out)

    def _ktx_version_string(self) -> str:
        if self._ktx_version is not None:
            return self._ktx_version

        exe = shutil.which(self.ktx_executable)
        if not exe:
            self._ktx_version = ""
            return self._ktx_version

        try:
            proc = subprocess.run(
                [exe, "--version"],
                capture_output=True,
                text=True,
                check=False,
                timeout=30,
            )
            self._ktx_version = (proc.stdout or proc.stderr or "").strip()
        except OSError:
            self._ktx_version = ""

        return self._ktx_version

    def _cache_path(
        self,
        kind: str,
        sources: Sequence[Path],
        *,
        vk_format: str,
        extra_ktx_args: tuple[str, ...],
        stem_fn: CacheStemFn | None,
    ) -> Path:
        stemmer = stem_fn or self.cache_stem_fn or default_cache_stem
        options: dict[str, Any] = {
            "vk_format": vk_format,
            "extra_ktx_args": list(extra_ktx_args),
            "ktx_version": self._ktx_version_string(),
        }
        stem = stemmer(kind, sources, options)
        if not stem.endswith(".ktx2"):
            stem = f"{stem}.ktx2"
        return self._cache_dir / stem

    def _build_create_command_2d(
        self,
        source: Path,
        output: Path,
        vk_format: str,
        extra: tuple[str, ...],
    ) -> list[str]:
        return [
            "create",
            "--format",
            vk_format,
            *extra,
            str(source),
            str(output),
        ]

    def _build_create_command_cube(
        self,
        faces: Sequence[Path],
        output: Path,
        vk_format: str,
        extra: tuple[str, ...],
    ) -> list[str]:
        return [
            "create",
            "--cubemap",
            "--format",
            vk_format,
            *extra,
            *[str(f) for f in faces],
            str(output),
        ]

    def _build_create_command_3d(
        self,
        slices: Sequence[Path],
        output: Path,
        vk_format: str,
        extra: tuple[str, ...],
    ) -> list[str]:
        depth = len(slices)
        return [
            "create",
            "--depth",
            str(depth),
            "--format",
            vk_format,
            *extra,
            *[str(s) for s in slices],
            str(output),
        ]

    def _convert_with_cli(
        self,
        final_path: Path,
        build_args: Callable[[Path], list[str]],
    ) -> None:
        final_path.parent.mkdir(parents=True, exist_ok=True)

        fd, tmp_name = tempfile.mkstemp(
            suffix=".ktx2.tmp",
            prefix="ktx_",
            dir=final_path.parent,
        )
        os.close(fd)
        tmp_path = Path(tmp_name)

        try:
            self._run_ktx(build_args(tmp_path))
            os.replace(tmp_path, final_path)
            logger.info("Wrote converted KTX: %s", final_path)
        except Exception:
            tmp_path.unlink(missing_ok=True)
            raise

    def _run_ktx(self, args: list[str]) -> None:
        exe = shutil.which(self.ktx_executable)
        if not exe:
            raise KtxToolNotFoundError(
                f"KTX CLI not found (executable={self.ktx_executable!r}). "
                "Install Khronos KTX-Software and ensure `ktx` is on PATH."
            )

        cmd = [exe, *args]
        logger.debug("Running KTX CLI: %s", " ".join(cmd))

        proc = subprocess.run(cmd, capture_output=True, text=True, check=False)
        if proc.returncode != 0:
            raise KtxToolInvocationError(
                f"KTX CLI failed with exit code {proc.returncode}",
                command=cmd,
                returncode=proc.returncode,
                stderr=proc.stderr or proc.stdout or "",
            )
