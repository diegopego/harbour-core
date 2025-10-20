#!/usr/bin/env python3
"""
Prototype CLI for Harbour AST-powered refactoring experiments.

This tool wraps the `hb_compAstTraceDumpJson()` payload emitted by the Harbour
compiler (`harbour --ast-trace --ast-trace-dump=-`) and produces rename /
extract plans shaped like VS Code LSP responses. It is intentionally limited
and meant for schema and workflow validation rather than production use.
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
import textwrap
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Tuple


@dataclass(frozen=True)
class Position:
    line: int  # 1-based
    column: int  # 1-based

    def to_lsp(self) -> Dict[str, int]:
        return {"line": self.line - 1, "character": self.column - 1}


@dataclass(frozen=True)
class Range:
    start: Position
    end: Position

    def to_lsp(self) -> Dict[str, Dict[str, int]]:
        return {"start": self.start.to_lsp(), "end": self.end.to_lsp()}


class TraceLoadError(RuntimeError):
    pass


def parse_position(value: str) -> Position:
    try:
        line_str, column_str = value.split(":")
        line = int(line_str, 10)
        column = int(column_str, 10)
    except ValueError as exc:
        raise argparse.ArgumentTypeError(
            "Position must follow the pattern LINE:COLUMN (1-based)."
        ) from exc
    if line < 1 or column < 1:
        raise argparse.ArgumentTypeError("Line and column must be >= 1.")
    return Position(line=line, column=column)


def parse_range(value: str) -> Range:
    try:
        start_str, end_str = value.split("-", 1)
        return Range(start=parse_position(start_str), end=parse_position(end_str))
    except ValueError as exc:
        raise argparse.ArgumentTypeError(
            "Range must follow the pattern START_LINE:START_COL-END_LINE:END_COL (1-based)."
        ) from exc


def normalise_module(module: Optional[str], cwd: Path) -> Optional[Path]:
    if module in (None, ""):
        return None
    candidate = Path(module)
    if not candidate.is_absolute():
        candidate = (cwd / candidate).resolve()
    return candidate


def load_trace_data(
    source: Path,
    harbour_path: Path,
    includes: Iterable[str],
    trace_path: Optional[Path],
) -> Dict[str, object]:
    if trace_path:
        try:
            with trace_path.open("r", encoding="utf-8") as handle:
                return json.load(handle)
        except FileNotFoundError as exc:
            raise TraceLoadError(f"Trace file not found: {trace_path}") from exc
        except json.JSONDecodeError as exc:
            raise TraceLoadError(f"Trace JSON is invalid: {trace_path}") from exc

    cmd: List[str] = [
        str(harbour_path),
        "--ast-trace",
        "--ast-trace-dump=-",
        str(source),
    ]
    for include in includes:
        cmd.insert(-1, f"-I{include}")

    try:
        result = subprocess.run(
            cmd,
            check=True,
            text=True,
            capture_output=True,
        )
    except FileNotFoundError as exc:
        raise TraceLoadError(f"Harbour compiler not found: {harbour_path}") from exc
    except subprocess.CalledProcessError as exc:
        message = exc.stderr.strip() or exc.stdout.strip()
        raise TraceLoadError(
            f"Harbour invocation failed with exit code {exc.returncode}: {message}"
        ) from exc

    stdout = result.stdout
    start = stdout.find("{")
    end = stdout.rfind("}")
    if start != -1 and end != -1:
        stdout = stdout[start : end + 1]
    try:
        return json.loads(stdout)
    except json.JSONDecodeError as exc:
        raise TraceLoadError(
            "Failed to parse JSON emitted by hb_compAstTraceDumpJson()."
        ) from exc


def find_token_at_position(
    trace_tokens: List[Dict[str, object]],
    module_path: Path,
    position: Position,
    cwd: Path,
) -> Optional[Dict[str, object]]:
    for token in trace_tokens:
        token_module = normalise_module(token.get("module"), cwd)
        if token_module != module_path:
            continue
        line = int(token.get("line", 0))
        column = int(token.get("column", 0))
        end_column = int(token.get("endColumn", 0))
        if line == 0 or column == 0 or end_column == 0:
            continue
        if line != position.line:
            continue
        if column <= position.column < end_column:
            return token
    return None


def build_workspace_edit(
    edits: Dict[Path, List[Range]],
    replacement_text: str,
) -> Dict[str, Dict[str, List[Dict[str, object]]]]:
    changes: Dict[str, List[Dict[str, object]]] = {}
    for module, ranges in edits.items():
        doc_path = module.resolve()
        key = doc_path.as_posix()
        changes.setdefault(key, [])
        for rng in ranges:
            changes[key].append(
                {
                    "range": rng.to_lsp(),
                    "newText": replacement_text,
                }
            )
    return {"changes": changes}


def token_overlaps_selection(token: Dict[str, object], selection: Range) -> bool:
    line = int(token.get("line", 0))
    column = int(token.get("column", 0))
    end_column = int(token.get("endColumn", 0))
    if line == 0 or column == 0 or end_column == 0:
        return False
    if line < selection.start.line or line > selection.end.line:
        return False
    if line == selection.start.line and end_column <= selection.start.column:
        return False
    if line == selection.end.line and column >= selection.end.column:
        return False
    return True


def build_function_scopes(
    trace_tokens: List[Dict[str, object]],
    module_path: Path,
    cwd: Path,
) -> List[Tuple[int, int]]:
    module_tokens = [
        token
        for token in trace_tokens
        if normalise_module(token.get("module"), cwd) == module_path
    ]
    module_tokens.sort(key=lambda token: int(token.get("sequence", 0)))
    scopes: List[Tuple[int, int]] = []
    current_start: Optional[int] = None
    prev_sequence: Optional[int] = None
    for token in module_tokens:
        sequence = int(token.get("sequence", 0))
        value = token.get("value")
        upper_value = value.upper() if isinstance(value, str) else ""
        if upper_value in {"FUNCTION", "PROCEDURE"}:
            if current_start is not None and prev_sequence is not None:
                scopes.append((current_start, prev_sequence))
            current_start = sequence
        prev_sequence = sequence
    if current_start is not None and prev_sequence is not None:
        scopes.append((current_start, prev_sequence))
    scopes.sort(key=lambda scope: (scope[1] - scope[0], scope[0]))
    return scopes


def find_enclosing_scope(
    scopes: List[Tuple[int, int]], sequence: int
) -> Optional[Tuple[int, int]]:
    for enter_sequence, leave_sequence in scopes:
        if enter_sequence <= sequence <= leave_sequence:
            return (enter_sequence, leave_sequence)
    return None


def is_definition_token(
    module_tokens: List[Dict[str, object]], index: int
) -> bool:
    token_line = int(module_tokens[index].get("line", 0))
    has_open_paren_ahead = any(
        t.get("value") == "(" and int(t.get("line", 0)) == token_line
        for t in module_tokens[index + 1 : index + 4]
    )
    for j in range(index - 1, -1, -1):
        prev = module_tokens[j]
        if int(prev.get("line", 0)) != token_line:
            break
        value = prev.get("value")
        if not isinstance(value, str):
            continue
        upper = value.upper()
        if upper in {"FUNCTION", "PROCEDURE"}:
            return True
        if upper == "STATIC" and has_open_paren_ahead:
            return True
    return False


def compute_references(
    traces: List[Tuple[Dict[str, object], Optional[Path]]],
    symbol: str,
    cwd: Path,
) -> Dict[str, object]:
    references: List[Dict[str, object]] = []
    for trace, _ in traces:
        tokens: List[Dict[str, object]] = trace.get("tokens", [])
        modules: Dict[Path, List[Dict[str, object]]] = {}
        for token in tokens:
            module_path = normalise_module(token.get("module"), cwd)
            if module_path is None:
                continue
            modules.setdefault(module_path, []).append(token)

        for module_path, module_tokens in modules.items():
            module_tokens.sort(key=lambda token: int(token.get("sequence", 0)))
            for idx, token in enumerate(module_tokens):
                if token.get("value") != symbol:
                    continue
                if is_definition_token(module_tokens, idx):
                    continue
                references.append(
                    {
                        "module": module_path.as_posix(),
                        "line": int(token.get("line", 0)),
                        "column": int(token.get("column", 0)),
                        "endColumn": int(token.get("endColumn", 0)),
                        "sequence": int(token.get("sequence", 0)),
                    }
                )

    references.sort(key=lambda ref: (ref["module"], ref["line"], ref["column"]))
    return {
        "kind": "references",
        "symbol": symbol,
        "references": references,
    }


def compute_rename(
    trace: Dict[str, object],
    source: Path,
    position: Position,
    new_name: str,
    cwd: Path,
) -> Dict[str, object]:
    tokens: List[Dict[str, object]] = trace.get("tokens", [])
    source_path = source.resolve()
    token = find_token_at_position(tokens, source_path, position, cwd)
    if token is None:
        raise TraceLoadError(
            f"No token found at {source}:{position.line}:{position.column}"
        )

    original_text = token.get("value")
    if not isinstance(original_text, str) or not original_text:
        raise TraceLoadError(
            "Target token does not carry a renameable lexeme (value is empty)."
        )

    token_sequence = int(token.get("sequence", 0))
    function_scopes = build_function_scopes(tokens, source_path, cwd)
    enclosing_scope = find_enclosing_scope(function_scopes, token_sequence)

    affected_ranges: Dict[Path, List[Range]] = {source_path: []}
    for candidate in tokens:
        candidate_module = normalise_module(candidate.get("module"), cwd)
        if candidate_module != source_path:
            continue
        if candidate.get("value") != original_text:
            continue
        if enclosing_scope is not None:
            candidate_sequence = int(candidate.get("sequence", 0))
            if not (
                enclosing_scope[0] <= candidate_sequence <= enclosing_scope[1]
            ):
                continue
        line = int(candidate.get("line", 0))
        column = int(candidate.get("column", 0))
        end_column = int(candidate.get("endColumn", 0))
        if line == 0 or column == 0 or end_column == 0:
            continue
        affected_ranges[source_path].append(
            Range(
                start=Position(line=line, column=column),
                end=Position(line=line, column=end_column),
            )
        )

    if not affected_ranges[source_path]:
        raise TraceLoadError("No candidate occurrences found for rename.")

    metadata: Dict[str, object] = {
        "occurrenceCount": len(affected_ranges[source_path]),
    }
    if enclosing_scope is not None:
        metadata["functionScope"] = {
            "startSequence": enclosing_scope[0],
            "endSequence": enclosing_scope[1],
        }

    return {
        "kind": "rename",
        "oldText": original_text,
        "newText": new_name,
        "workspaceEdit": build_workspace_edit(affected_ranges, new_name),
        "metadata": metadata,
    }


def compute_extract(
    trace: Dict[str, object],
    source: Path,
    selection: Range,
    new_name: str,
    insert_line: Optional[int],
    cwd: Path,
) -> Dict[str, object]:
    source_path = source.resolve()
    file_text = source_path.read_text(encoding="utf-8")
    lines = file_text.splitlines()

    if selection.start.line < 1 or selection.end.line > len(lines):
        raise TraceLoadError("Selection lines fall outside the source buffer.")

    if selection.start.line > selection.end.line or (
        selection.start.line == selection.end.line
        and selection.start.column >= selection.end.column
    ):
        raise TraceLoadError("Selection range must be non-empty.")

    excerpt = slice_range_text(lines, selection)
    excerpt_dedented = textwrap.dedent(excerpt)
    call_indent = " " * (selection.start.column - 1)

    stripped_lines = [line.lstrip() for line in excerpt_dedented.splitlines() if line.strip()]
    tail = stripped_lines[-1] if stripped_lines else ""
    if tail.upper().startswith("RETURN "):
        replacement_text = f"{call_indent}RETURN {new_name}()"
    else:
        replacement_text = f"{call_indent}{new_name}()"
    if selection.start.column == 1:
        replacement_text += "\n"

    insertion_line = insert_line or len(lines) + 1
    insertion_position = Position(line=insertion_line, column=1)

    tokens: List[Dict[str, object]] = trace.get("tokens", [])
    selected_tokens = [
        token
        for token in tokens
        if normalise_module(token.get("module"), cwd) == source_path
        and token_overlaps_selection(token, selection)
    ]
    if not selected_tokens:
        raise TraceLoadError(
            "Selection does not intersect any trace tokens; verify the range arguments."
        )

    new_function_body = build_extract_function_body(
        new_name=new_name,
        selection_text=excerpt_dedented,
    )

    edits: Dict[Path, List[Range]] = {source_path: [selection]}
    workspace_edit = build_workspace_edit(edits, replacement_text)
    module_key = source_path.resolve().as_posix()
    workspace_edit.setdefault("changes", {})
    workspace_edit["changes"].setdefault(module_key, []).append(
        {
            "range": {
                "start": {"line": insertion_position.line - 1, "character": 0},
                "end": {"line": insertion_position.line - 1, "character": 0},
            },
            "newText": ensure_trailing_newline(new_function_body),
        }
    )

    return {
        "kind": "extract",
        "newSymbol": new_name,
        "workspaceEdit": workspace_edit,
        "metadata": {
            "selection": selection.to_lsp(),
            "insertionLine": insertion_line,
            "selectedTokenCount": len(selected_tokens),
        },
    }


def ensure_trailing_newline(block: str) -> str:
    return block if block.endswith("\n") else f"{block}\n"


def slice_range_text(lines: List[str], selection: Range) -> str:
    start_idx = selection.start.line - 1
    end_idx = selection.end.line - 1
    pieces: List[str] = []
    base_indent = max(selection.start.column - 1, 0)

    if start_idx == end_idx:
        line = lines[start_idx]
        fragment = line[selection.start.column - 1 : selection.end.column - 1]
        if base_indent:
            fragment = (" " * base_indent) + fragment
        pieces.append(fragment)
    else:
        first_line = lines[start_idx]
        first_fragment = first_line[selection.start.column - 1 :]
        if base_indent:
            first_fragment = (" " * base_indent) + first_fragment
        pieces.append(first_fragment)
        for idx in range(start_idx + 1, end_idx):
            pieces.append(lines[idx])
        last_line = lines[end_idx]
        pieces.append(last_line[: selection.end.column - 1])

    return "\n".join(pieces)


def build_extract_function_body(new_name: str, selection_text: str) -> str:
    indented_body = indent_block(selection_text, "   ")
    meaningful_lines = [
        line.strip().upper()
        for line in selection_text.splitlines()
        if line.strip()
    ]
    has_terminal_return = bool(meaningful_lines) and meaningful_lines[-1].startswith(
        "RETURN"
    )
    body_line = (
        indented_body if indented_body.strip() else "   // TODO: populate body"
    )
    lines = [
        "",
        f"FUNCTION {new_name}()",
        body_line,
    ]
    if not has_terminal_return:
        lines.append("   RETURN NIL")
    return "\n".join(lines)


def indent_block(block: str, prefix: str) -> str:
    return "\n".join(prefix + line if line else "" for line in block.splitlines())


def main(argv: Optional[List[str]] = None) -> int:
    parser = argparse.ArgumentParser(
        description="Prototype refactoring CLI consuming Harbour AST trace JSON."
    )
    parser.add_argument(
        "--harbour",
        type=Path,
        default=Path("bin/linux/gcc/harbour"),
        help="Path to the Harbour compiler executable.",
    )
    parser.add_argument(
        "--trace",
        type=Path,
        action="append",
        help="Existing hb_compAstTraceDumpJson() output(s) to consume instead of invoking Harbour.",
    )
    parser.add_argument(
        "-I",
        "--include",
        dest="includes",
        action="append",
        default=[],
        help="Additional include directories passed to Harbour (-Ipath).",
    )
    parser.add_argument(
        "--pretty",
        action="store_true",
        help="Pretty-print JSON output.",
    )

    subparsers = parser.add_subparsers(dest="command", required=True)

    rename_parser = subparsers.add_parser(
        "rename", help="Produce a rename WorkspaceEdit payload."
    )
    rename_parser.add_argument("source", type=Path, help="Source file to analyse.")
    rename_parser.add_argument(
        "--position",
        required=True,
        type=parse_position,
        help="1-based LINE:COLUMN identifying the symbol occurrence to rename.",
    )
    rename_parser.add_argument(
        "--new-name",
        required=True,
        help="Replacement identifier to use.",
    )

    extract_parser = subparsers.add_parser(
        "extract", help="Produce a function-extraction WorkspaceEdit payload."
    )
    extract_parser.add_argument("source", type=Path, help="Source file to analyse.")
    extract_parser.add_argument(
        "--range",
        required=True,
        type=parse_range,
        help="1-based START_LINE:START_COL-END_LINE:END_COL selection to extract.",
    )
    extract_parser.add_argument(
        "--new-name",
        required=True,
        help="Name of the extracted function.",
    )
    extract_parser.add_argument(
        "--insert-line",
        type=int,
        help="1-based line at which to insert the new function (defaults to EOF).",
    )

    references_parser = subparsers.add_parser(
        "references", help="List symbol references across source files."
    )
    references_parser.add_argument(
        "--symbol",
        required=True,
        help="Identifier to search for.",
    )
    references_parser.add_argument(
        "sources",
        nargs="+",
        type=Path,
        help="Source files to analyse for references.",
    )

    args = parser.parse_args(argv)
    cwd = Path.cwd()

    try:
        trace_paths = args.trace or []
        if args.command == "rename":
            trace_path = trace_paths[0] if trace_paths else None
            trace = load_trace_data(
                source=args.source,
                harbour_path=args.harbour,
                includes=args.includes,
                trace_path=trace_path,
            )
            payload = compute_rename(
                trace=trace,
                source=args.source,
                position=args.position,
                new_name=args.new_name,
                cwd=cwd,
            )
        elif args.command == "extract":
            trace_path = trace_paths[0] if trace_paths else None
            trace = load_trace_data(
                source=args.source,
                harbour_path=args.harbour,
                includes=args.includes,
                trace_path=trace_path,
            )
            payload = compute_extract(
                trace=trace,
                source=args.source,
                selection=args.range,
                new_name=args.new_name,
                insert_line=args.insert_line,
                cwd=cwd,
            )
        elif args.command == "references":
            traces: List[Tuple[Dict[str, object], Optional[Path]]] = []
            if trace_paths:
                for path in trace_paths:
                    traces.append((load_trace_data(path, args.harbour, args.includes, path), None))
            else:
                for source_path in args.sources:
                    trace = load_trace_data(
                        source=source_path,
                        harbour_path=args.harbour,
                        includes=args.includes,
                        trace_path=None,
                    )
                    traces.append((trace, source_path))
            payload = compute_references(
                traces=traces,
                symbol=args.symbol,
                cwd=cwd,
            )
        else:
            parser.error(f"Unsupported command: {args.command}")

    except TraceLoadError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    if args.pretty:
        json.dump(payload, sys.stdout, indent=2, sort_keys=True)
        sys.stdout.write("\n")
    else:
        json.dump(payload, sys.stdout)
        sys.stdout.write("\n")

    return 0


if __name__ == "__main__":
    sys.exit(main())
