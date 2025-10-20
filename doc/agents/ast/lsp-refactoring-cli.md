# LSP Rename/Extract Prototype

## Goals
- Validate that the compiler-backed AST trace (`hb_compAstTraceDumpJson()`) exposes enough information to drive basic refactorings.
- Exercise the VS Code LSP data contract for rename and extract-style code actions using Harbour sources.
- Provide a hand-off artefact for the AST Tooling agent so schema changes remain coordinated across downstream consumers.

## VS Code Data Contract Overview

### Rename (`textDocument/rename`)
- **Request payload**:
  - `textDocument`: URI of the source buffer (`file:///path/to/foo.prg`).
  - `position`: zero-based `{ line, character }` pointing to the identifier occurrence.
  - `newName`: replacement identifier.
- **Expected response**: a `WorkspaceEdit`. Harbour tooling can rely on the simple `changes` form (map of document URI → `TextEdit[]`) because rename does not require versioned edits.
- **Trace fields consumed**:
  - `tokens[].value` to match the original lexeme.
  - `tokens[].module`, `line`, `column`, `endColumn` (converted to zero-based) to build edit ranges.
  - `tokens[].sequence` (implicitly, because the trace preserves event ordering).
- **Current constraints**:
  - Scope resolution is lexical: tokens are renamed only inside the containing `FUNCTION`/`PROCEDURE` block. Macro expansions (`module == null`) are ignored.
  - Token-type filtering is not yet implemented; identifier vs literal disambiguation relies on follow-up schema work.

### Extract (`codeAction` → `codeAction/resolve` → `workspace/applyEdit`)
- **Request payload** (per VS Code command infrastructure):
  - `textDocument`: source buffer URI.
  - `range`: selection to extract (zero-based).
  - Optional command arguments (`newName`, insertion hints).
- **Response shape**: a `WorkspaceEdit` describing both the replacement of the original selection and the insertion of the new function/procedure.
- **Trace fields consumed**:
  - `tokens[].module`, `line`, `column`, `endColumn` to confirm the selection intersects compiler-observed tokens.
  - No node-level data is consumed yet; pairing ENTER/LEAVE events is left for a future iteration when we need scope validation.
- **Current constraints**:
  - Selections must fall within a single source module and intersect at least one token.
  - Indentation is normalised relative to the selection start; mixed tabs are not yet treated.
  - Fallback `RETURN NIL` insertion only happens when the extracted block does not already end with a `RETURN`.

## CLI Prototype (`tests/ast/python/ast_refactor_cli.py`)

### Capabilities
- Emits rename and extract plans as JSON documents already shaped like a VS Code `WorkspaceEdit`.
- Consumes an existing trace dump via `--trace` or spawns the compiler directly (`harbour --ast-trace --ast-trace-dump=-`).
- Logs structured metadata (`occurrenceCount`, `selectedTokenCount`, insertion line, `functionScope`) that downstream integrations can use for previews.
- Lists symbol references across one or more source files (`references` subcommand).

### Usage

```sh
# Rename the identifier at line 4, column 10 in fixture_demo.prg.
python3 tests/ast/python/ast_refactor_cli.py \
  --trace tests/ast/fixtures/fixture_demo.ast.json \
  rename tests/ast/fixture_demo.prg \
  --position 4:10 \
  --new-name DemoRenamed \
  --pretty

# Extract lines 5-6 into a new function called DemoBody.
python3 tests/ast/python/ast_refactor_cli.py \
  --trace tests/ast/fixtures/fixture_demo.ast.json \
  extract tests/ast/fixture_demo.prg \
  --range 5:4-6:24 \
  --new-name DemoBody \
  --pretty

# Locate calls to SupportFunc() across two modules.
python3 tests/ast/python/ast_refactor_cli.py \
  references --symbol SupportFunc \
  tests/ast/ref_project/main.prg \
  tests/ast/ref_project/support.prg
```

### Output
- All commands emit JSON with the keys:
  - `kind`: `"rename"`, `"extract"`, or `"references"`.
  - `workspaceEdit`: LSP-compatible `WorkspaceEdit` (rename/extract commands).
  - `metadata`: helper fields for tooling (counts, selection range, insertion line, scope boundaries).
    - Rename responses include `functionScope.startSequence` / `endSequence` when the identifier belongs to a function/procedure.
  - `references`: for the reference finder, an array of `{module, line, column, endColumn, sequence}` entries sorted by module and position.
- `workspaceEdit.changes` holds text edits keyed by absolute POSIX paths (per VS Code expectations).
- Extraction appends the new function at the requested line (defaults to EOF) and normalises indentation relative to the selection start.
- To materialise edits for inspection, use `tests/ast/python/apply_workspace_edit.py`:

  ```sh
  python3 tests/ast/python/ast_refactor_cli.py --pretty \
    --trace tests/ast/fixtures/fixture_demo.ast.json \
    rename tests/ast/fixture_demo.prg \
    --position 4:10 \
    --new-name DemoRenamed \
    > /tmp/rename.json

  python3 tests/ast/python/apply_workspace_edit.py \
    --edit /tmp/rename.json \
    --source tests/ast/fixture_demo.prg \
    --output /tmp/fixture_demo.renamed.prg
  diff -u tests/ast/fixture_demo.prg /tmp/fixture_demo.renamed.prg
  ```

- Example output for the sequence above:

  ```diff
  --- tests/ast/fixture_demo.prg
  +++ /tmp/fixture_demo.renamed.prg
  @@
  -FUNCTION Demo()
  +FUNCTION DemoRenamed()
     LOCAL n := VALUE
     RETURN Helper() + n
  ```

### Tests
- `tests/ast/python/test_refactor_cli.py` provides pytest-based coverage against the `fixture_demo` fixtures and the `ref_project` multi-module sample. `tests/ast/python/test-python.sh` runs the Python suite and is invoked from `tests/ast/Makefile`, so `scripts/test-ast.sh` executes it alongside the cmocka binaries.
- The pytest module rewrites `tests/ast/fixture_demo.rename.prg`, `tests/ast/fixture_demo.helper_scope.prg`, and `tests/ast/fixture_demo.extract.prg` using the CLI output and asserts that the emitted content matches the checked-in fixtures, leaving the refactored `.prg` copies available for inspection.
- Reference tests rely on `tests/ast/ref_project/*.prg` and ensure that the CLI reports every call site without misclassifying function definitions.

## Schema Alignment Notes
- Downstream refactoring logic currently relies only on stable fields documented in `doc/agents/ast/serialization-format.md`:
  - `tokens[].value`, `module`, `line`, `column`, `endColumn`.
  - Future iterations will introduce node-scope checks once `HB_COMP_AST_NODE_KIND` covers inline/init/exit scenarios.
- Any changes to those fields must be coordinated with the AST Tooling agent; update this document alongside serialization docs.
- Tests/fixtures to add in subsequent sessions:
  - Rename across macro-expanded identifiers (requires schema support to surface expansion ancestry).
  - Extraction that spans nested scopes (`IF/ELSE`, `FOR/NEXT`) once node pairing of ENTER/LEAVE is in place.

## Next Steps
- Enrich rename analysis with token-type filtering and node ancestry to avoid literal collisions.
- Replace the current lexical function scoping with AST-backed boundaries (node events / symbol resolution) so extraction and rename can validate containment before proposing edits.
