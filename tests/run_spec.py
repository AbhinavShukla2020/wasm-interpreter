from __future__ import annotations

import argparse
import json
import subprocess
import tempfile
from pathlib import Path


def value_argument(value: dict) -> str:
    if value["type"] != "i32":
        raise ValueError("runner currently supports i32 arguments")
    raw = int(value["value"])
    return str(raw if raw < 2**31 else raw - 2**32)


def main() -> None:
    parser = argparse.ArgumentParser(description="Run supported official spec assertions")
    parser.add_argument("wast", type=Path)
    parser.add_argument("--runtime", default="build/miniwm")
    args = parser.parse_args()
    passed = failed = skipped = 0
    with tempfile.TemporaryDirectory() as directory:
        manifest = Path(directory) / "suite.json"
        subprocess.run(["wast2json", str(args.wast), "-o", str(manifest)], check=True)
        suite = json.loads(manifest.read_text())
        current_module = None
        for command in suite["commands"]:
            if command["type"] == "module":
                current_module = Path(directory) / command["filename"]
                continue
            if command["type"] not in {"assert_return", "assert_trap"} or current_module is None:
                skipped += 1
                continue
            action = command.get("action", {})
            if action.get("type") != "invoke" or any(arg["type"] != "i32" for arg in action.get("args", [])):
                skipped += 1
                continue
            process = subprocess.run(
                [args.runtime, str(current_module), action["field"], *(value_argument(v) for v in action.get("args", []))],
                capture_output=True,
                text=True,
            )
            expected_trap = command["type"] == "assert_trap"
            success = process.returncode != 0 if expected_trap else process.returncode == 0
            if success and not expected_trap and command.get("expected"):
                success = process.stdout.strip() == value_argument(command["expected"][0])
            passed += int(success)
            failed += int(not success)
    print(json.dumps({"passed": passed, "failed": failed, "skipped": skipped}))
    raise SystemExit(1 if failed else 0)


if __name__ == "__main__":
    main()

