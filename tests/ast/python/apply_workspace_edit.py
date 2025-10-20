#!/usr/bin/env python3
"""
Apply a VS Code-style WorkspaceEdit JSON payload to a Harbour source file copy.

The script is intended to help inspect refactoring results produced by
`scripts/ast_refactor_cli.py`. By default it writes edits to a separate output
path so the original file remains untouched (which makes `diff` workflows
straightforward).
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Tuple


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Apply WorkspaceEdit JSON to a Harbour source file."
    )
    parser.add_argument(
        "--edit",
        required=True,
        type=Path,
        help="Path to the JSON file containing the WorkspaceEdit payload.",
    )
    parser.add_argument(
        "--source",
        required=True,
        type=Path,
        help="Path to the source file to transform.",
    )
    parser.add_argument(
        "--output",
        required=True,
        type=Path,
        help="Destination file that will receive the transformed content.",
    )
    parser.add_argument(
        "--document-uri",
        type=str,
        help=(
            "Optional document URI key inside the WorkspaceEdit. "
            "Defaults to the resolved source path."
        ),
    )
    return parser.parse_args()


def load_workspace_edit(path: Path) -> Dict[str, object]:
    with path.open("r", encoding="utf-8") as handle:
        payload = json.load(handle)
    if "workspaceEdit" in payload:
        payload = payload["workspaceEdit"]
    return payload


def collect_text_edits(
    workspace_edit: Dict[str, object],
    target_uri: str,
) -> List[Dict[str, object]]:
    edits: List[Dict[str, object]] = []

    changes = workspace_edit.get("changes", {})
    if isinstance(changes, dict):
        edits.extend(changes.get(target_uri, []))

    document_changes = workspace_edit.get("documentChanges", [])
    if isinstance(document_changes, list):
        for change in document_changes:
            if not isinstance(change, dict):
                continue
            if change.get("textDocument", {}).get("uri") == target_uri:
                edits.extend(change.get("edits", []))

    return edits


def position_to_offset(
    pos: Dict[str, int],
    line_offsets: List[int],
    line_lengths: List[int],
    text_length: int,
) -> int:
    line = pos["line"]
    character = pos["character"]
    if line >= len(line_offsets):
        return text_length
    return line_offsets[line] + min(character, line_lengths[line])


def apply_edits(text: str, edits: Iterable[Dict[str, object]]) -> str:
    lines = text.splitlines(keepends=True)
    line_offsets: List[int] = []
    running = 0
    line_lengths: List[int] = []
    for line in lines:
        line_offsets.append(running)
        line_lengths.append(len(line))
        running += len(line)
    text_length = len(text)

    normalised_edits: List[Tuple[int, int, str]] = []
    for edit in edits:
        rng = edit.get("range")
        if not isinstance(rng, dict):
            continue
        start = position_to_offset(
            rng.get("start", {"line": 0, "character": 0}),
            line_offsets,
            line_lengths,
            text_length,
        )
        end = position_to_offset(
            rng.get("end", {"line": 0, "character": 0}),
            line_offsets,
            line_lengths,
            text_length,
        )
        new_text = edit.get("newText", "")
        normalised_edits.append((start, end, new_text))

    # Apply edits from the end of the file backwards so offsets stay valid.
    normalised_edits.sort(key=lambda item: item[0], reverse=True)
    result = text
    for start, end, new_text in normalised_edits:
        result = result[:start] + new_text + result[end:]
    return result


def apply_workspace_edit_payload(
    workspace_edit: Dict[str, object],
    source_path: Path,
    output_path: Path,
    *,
    document_uri: Optional[str] = None,
) -> int:
    resolved_source = source_path.resolve()
    resolved_output = output_path.resolve()
    target_uri = document_uri if document_uri else resolved_source.as_posix()
    edits = collect_text_edits(workspace_edit, target_uri)
    if not edits:
        raise SystemExit(
            f"No edits found for document URI {target_uri!r} in workspace edit."
        )

    text = resolved_source.read_text(encoding="utf-8")
    transformed = apply_edits(text, edits)
    resolved_output.parent.mkdir(parents=True, exist_ok=True)
    resolved_output.write_text(transformed, encoding="utf-8")
    return len(edits)


def main() -> int:
    args = parse_args()
    workspace_edit = load_workspace_edit(args.edit)
    edits_applied = apply_workspace_edit_payload(
        workspace_edit,
        args.source,
        args.output,
        document_uri=args.document_uri,
    )
    print(f"Applied {edits_applied} edit(s) to {Path(args.output).resolve()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
