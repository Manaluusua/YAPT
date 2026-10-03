"""
Project settings read from settings.cfg at the repo root, overridden per machine by an optional settings_local.cfg
next to it (gitignored).

ASSET_PATH is the root folder for models / textures. Relative asset paths given to the loaders (Resources,
SceneLoader) are resolved against it via resolve_asset_path(), absolute paths are used as is.
"""

import configparser
from pathlib import Path

SETTINGS_FILE = "settings.cfg"
SETTINGS_LOCAL_FILE = "settings_local.cfg"
DEFAULT_ASSET_PATH = "../YAPTAssets"

def find_project_root():
    """First folder above this file that holds settings.cfg, falls back to the repo root of the source tree."""
    here = Path(__file__).resolve()
    for folder in here.parents:
        if (folder / SETTINGS_FILE).is_file():
            return folder
    print(f"settings: {SETTINGS_FILE} not found above {here.parent}, using defaults")
    return here.parents[4] if len(here.parents) > 4 else here.parent

def _load_settings(root):
    config = configparser.ConfigParser()
    # later files override earlier ones
    read = config.read([root / SETTINGS_FILE, root / SETTINGS_LOCAL_FILE], encoding="utf-8")
    for file in read:
        print(f"settings: loaded {file}")
    return config

def _resolve_against(root, value):
    path = Path(value).expanduser()
    if not path.is_absolute():
        path = root / path
    return path.resolve()

PROJECT_ROOT = find_project_root()
_settings = _load_settings(PROJECT_ROOT)

ASSET_PATH = _resolve_against(PROJECT_ROOT, _settings.get("paths", "asset_path", fallback=DEFAULT_ASSET_PATH))
if not ASSET_PATH.is_dir():
    print(f"settings: asset path {ASSET_PATH} does not exist, set [paths] asset_path in {SETTINGS_LOCAL_FILE}")

def resolve_asset_path(path):
    """Absolute paths are returned as is, relative ones are resolved against ASSET_PATH."""
    if path is None or str(path) == "":
        return path
    p = Path(path)
    if p.is_absolute():
        return str(p)
    return str((ASSET_PATH / p).resolve())
