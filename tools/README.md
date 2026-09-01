# Tools

## Asset cooker
From the repository root:

```
python tools/cook_assets.py
```

Python 3, standard library only. No pip packages. No Blender.

The cooker copies known authored files from `game/assets/source/` to `game/assets/cooked/`, skips a rewrite when the SHA-256 content already matches, writes `game/assets/cooked/manifest.json`, and removes only previously manifested cooked outputs that are no longer in the known asset list.
