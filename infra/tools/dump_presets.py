"""Writes the presets defined in one or more preset files (written with the make_presets.py helpers) to a JSON file in
the presets.json format, for infra_audition --presets. Nothing else is touched.

    python3 dump_presets.py out.json presets_6_bass.py [...]
"""
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import make_presets as mp  # noqa: E402  (helpers only)

mp.PRESETS.clear()
for path in sys.argv[2:]:
    exec(compile(open(path).read(), path, "exec"), vars(mp))
with open(sys.argv[1], "w") as f:
    json.dump({"version": 1, "presets": mp.PRESETS}, f, indent=1)
print(f"{len(mp.PRESETS)} presets -> {sys.argv[1]}")
