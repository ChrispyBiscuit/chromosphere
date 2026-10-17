"""Instruction-count regression check (PRD 8.3 #10). Deterministic, so it works on noisy CI hosts.
Counts instructions executed inside Reverb::process for one second of audio under callgrind.
Usage: python3 tools/perf_check.py [--update]   (fails if any case is >5% above tools/perf_baseline.json)
"""
import json, os, re, subprocess, sys, tempfile

root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
exe = os.path.join(root, "build", "fr_profile_run")
base_path = os.path.join(root, "tools", "perf_baseline.json")
cases = ["eco", "normal", "high"]


def count(case):
    with tempfile.TemporaryDirectory() as d:
        out = os.path.join(d, "cg.out")
        r = subprocess.run(["valgrind", "--tool=callgrind", "--toggle-collect=fr::Reverb::processChunk*",
                            "--callgrind-out-file=" + out, exe, case], capture_output=True, text=True)
        m = re.search(r"Collected\s*:\s*(\d+)", r.stderr)
        return int(m.group(1))


now = {c: count(c) for c in cases}
if "--update" in sys.argv:
    json.dump(now, open(base_path, "w"), indent=2); print("baseline written:", now); sys.exit(0)
base = json.load(open(base_path))
bad = 0
for c in cases:
    pct = 100.0 * (now[c] / base[c] - 1)
    flag = "FAIL" if pct > 5 else "ok"
    bad += pct > 5
    print("%-7s %12d instr/s  baseline %12d  %+5.1f%%  %s" % (c, now[c], base[c], pct, flag))
sys.exit(1 if bad else 0)
