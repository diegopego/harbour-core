"""
Pytest smoke coverage for the AST-backed refactoring CLI.

The tests execute `scripts/ast_refactor_cli.py` against existing trace fixtures
and assert that the emitted WorkspaceEdit payloads match the expected VS Code
contract (rename and extract cases).
"""

from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

from .apply_workspace_edit import apply_workspace_edit_payload


ROOT = Path(__file__).resolve().parents[2]
CLI = ROOT / "scripts" / "ast_refactor_cli.py"
TRACE = ROOT / "tests" / "ast" / "fixtures" / "fixture_demo.ast.json"
SOURCE = ROOT / "tests" / "ast" / "fixture_demo.prg"
SOURCE_URI = SOURCE.resolve().as_posix()


def run_cli(arguments: list[str]) -> dict:
    result = subprocess.run(
        [sys.executable, str(CLI), *arguments],
        capture_output=True,
        text=True,
        check=True,
        cwd=ROOT,
    )
    return json.loads(result.stdout)


def test_rename_workspace_edit_payload() -> None:
    expected_fixture = RENAMED_FIXTURE.read_text(encoding="utf-8")
    payload = run_cli(
        [
            "--trace",
            str(TRACE),
            "rename",
            str(SOURCE),
            "--position",
            "4:10",
            "--new-name",
            "DemoRenamed",
        ]
    )

    assert payload["kind"] == "rename"
    assert payload["oldText"] == "Demo"
    assert payload["newText"] == "DemoRenamed"
    changes = payload["workspaceEdit"]["changes"]
    assert SOURCE_URI in changes
    edits = changes[SOURCE_URI]
    assert len(edits) == 1
    assert edits[0]["newText"] == "DemoRenamed"
    assert edits[0]["range"] == {
        "start": {"line": 3, "character": 9},
        "end": {"line": 3, "character": 13},
    }
    assert payload["metadata"]["occurrenceCount"] == 1

    edits_applied = apply_workspace_edit_payload(
        payload["workspaceEdit"],
        SOURCE,
        RENAMED_FIXTURE,
        document_uri=SOURCE_URI,
    )
    assert edits_applied == 1
    actual_fixture = RENAMED_FIXTURE.read_text(encoding="utf-8")
    assert actual_fixture == expected_fixture


def test_extract_workspace_edit_payload() -> None:
    expected_fixture = EXTRACT_FIXTURE.read_text(encoding="utf-8")
    payload = run_cli(
        [
            "--trace",
            str(TRACE),
            "extract",
            str(SOURCE),
            "--range",
            "5:4-6:24",
            "--new-name",
            "DemoBody",
        ]
    )

    assert payload["kind"] == "extract"
    assert payload["newSymbol"] == "DemoBody"
    changes = payload["workspaceEdit"]["changes"]
    assert SOURCE_URI in changes
    edits = changes[SOURCE_URI]
    assert len(edits) == 2
    call_edit, insert_edit = edits
    assert call_edit["newText"] == "   DemoBody()"
    assert call_edit["range"] == {
        "start": {"line": 4, "character": 3},
        "end": {"line": 5, "character": 23},
    }
    assert insert_edit["range"] == {
        "start": {"line": 22, "character": 0},
        "end": {"line": 22, "character": 0},
    }
    assert insert_edit["newText"] == (
        "\nFUNCTION DemoBody()\n   LOCAL n := VALUE\n   RETURN Helper() + n\n"
    )
    metadata = payload["metadata"]
    assert metadata["selection"] == {
        "start": {"line": 4, "character": 3},
        "end": {"line": 5, "character": 23},
    }
    assert metadata["selectedTokenCount"] == 10
    assert metadata["insertionLine"] == 23

    edits_applied = apply_workspace_edit_payload(
        payload["workspaceEdit"],
        SOURCE,
        EXTRACT_FIXTURE,
        document_uri=SOURCE_URI,
    )
    assert edits_applied == 2
    actual_fixture = EXTRACT_FIXTURE.read_text(encoding="utf-8")
    assert actual_fixture == expected_fixture
RENAMED_FIXTURE = ROOT / "tests" / "ast" / "fixture_demo.rename.prg"
EXTRACT_FIXTURE = ROOT / "tests" / "ast" / "fixture_demo.extract.prg"
