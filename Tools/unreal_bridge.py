"""Run editor Python through Epic's installed remote execution client."""
import argparse
import ast
import importlib.util
import json
import os
from pathlib import Path
import sys
import time

PROJECT = Path(__file__).resolve().parents[1] / "LOL_UNREAL"
DEFAULT_ENGINE = Path("C:/Program Files/Epic Games/UE_5.7")


def normalized(path):
    return os.path.normcase(os.path.realpath(path))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=["status", "exec"])
    parser.add_argument("--file", type=Path, help="UTF-8 Python script to execute in the editor")
    parser.add_argument("--engine", type=Path, default=DEFAULT_ENGINE)
    parser.add_argument("--timeout", type=float, default=8)
    args = parser.parse_args()
    if args.action == "exec" and args.file is None:
        parser.error("exec requires --file")
    script = args.file.read_text(encoding="utf-8-sig") if args.file else None
    module_path = args.engine / "Engine/Plugins/Experimental/PythonScriptPlugin/Content/Python/remote_execution.py"
    spec = importlib.util.spec_from_file_location("ue_remote_execution", module_path)
    remote = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(remote)
    session = remote.RemoteExecution()
    try:
        session.start()
        deadline = time.monotonic() + args.timeout
        matches = []
        while time.monotonic() < deadline:
            matches = [node for node in session.remote_nodes
                       if node.get("project_root")
                       and normalized(node["project_root"]) == normalized(PROJECT)]
            if matches:
                break
            time.sleep(0.2)
        if len(matches) != 1:
            raise RuntimeError("Expected one LOL_UNREAL editor, found %d. Open the project with Python Remote Execution enabled." % len(matches))
        session.open_command_connection(matches[0]["node_id"])
        # Verify the actual project again over the command channel before executing user code.
        probe = session.run_command(
            "unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())",
            exec_mode=remote.MODE_EVAL_STATEMENT)
        if not probe.get("success") or normalized(ast.literal_eval(probe["result"])) != normalized(PROJECT):
            raise RuntimeError("Editor project verification failed: %s" % probe)
        if args.action == "status":
            print(json.dumps({"connected": True, "project": str(PROJECT), "node": matches[0]}, ensure_ascii=False, indent=2))
            return 0
        result = session.run_command(script, exec_mode=remote.MODE_EXEC_FILE)
        result.pop('command', None)
        print(json.dumps(result, ensure_ascii=False, indent=2))
        return 0 if result.get("success") else 1
    finally:
        session.stop()


if __name__ == "__main__":
    sys.stdout.reconfigure(encoding="utf-8")
    sys.stderr.reconfigure(encoding="utf-8")
    try:
        sys.exit(main())
    except Exception as error:
        print(json.dumps({"connected": False, "error": str(error)}, ensure_ascii=False), file=sys.stderr)
        sys.exit(1)
