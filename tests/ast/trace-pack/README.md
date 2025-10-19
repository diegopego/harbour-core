# Harbour AST Trace Pack (2025-10-25)

Contents:
- Core fixtures (`fixture_*.prg`, helper `.ch` headers) covering declarations, control flow, preprocessor directives, expressions, include chains, and dialect pragmas.
- Matching compiler-generated JSON traces (`fixture_*.ast.json`) captured via `--ast-trace` / `--ast-trace-dump`.

Regeneration workflow:
1. Run the commands listed in `doc/agents/ast/instrumentation-plan.md` (Snapshot Regeneration Workflow) to refresh the JSON dumps.
2. Verify the harness with `tests/ast/hbmk-ast-tests` and `./tests/ast/compilebuf-tests`.
3. Repackage:
   ```
   zip -j tests/ast/trace-pack/core-trace-pack-2025-10-25.zip tests/ast/fixture_*.prg tests/ast/fixture_*.ch tests/ast/fixtures/fixture_*.ast.json
   ```

The archive flattens paths so consumers can extract the files directly into their fixture directory of choice.
