## AST instrumentation quick reference

Harbour’s AST tooling now rides entirely on the compiler trace adapter (see commit `4da14975e24335cf02db6e340d4b0ec2e8ae75ce`). There is no separate lexer, builder, or utility binary to maintain — the compiler emits the authoritative trace.

### Building once

```sh
make HB_BUILD_PARTS=compiler HB_PLATFORM=linux HB_COMPILER=gcc
```

This produces `bin/<platform>/<compiler>/harbour` with the trace hooks enabled (`hbtraceast.c`, `--ast-trace` flags, diagnostics counters).

### Capturing a trace

```sh
bin/linux/gcc/harbour -iinclude --ast-trace --ast-trace-dump=- tests/ast/fixture_demo.prg
```

The compiler streams a JSON event log containing the token stream, lexer boundary events, parser node enter/leave events, and preprocessor trace information. Redirect `--ast-trace-dump` to a file when you need a fixture.

### Diagnostics mode

Instrumentation counters stay off by default. Enable them when troubleshooting:

```sh
bin/linux/gcc/harbour -iinclude --ast-trace \
  --ast-trace-diagnostics --ast-trace-dump=- tests/ast/fixture_demo.prg
```

`--ast-trace-diagnostics` (or the matching `HB_AST_TRACE_DIAGNOSTICS` env toggle) records total tokens, nodes, boundaries, PP events, and traceinfo retain/release counts without keeping the event payload in memory.

### Refreshing trace fixtures

```sh
for prg in fixture_demo fixture_blocks fixture_ppdirectives fixture_statements \
           fixture_expressions fixture_includes fixture_compat_clipper fixture_compat_harbour \
           fixture_macro_expansion fixture_inline_real; do
  bin/linux/gcc/harbour -iinclude --ast-trace \
    --ast-trace-dump=tests/ast/fixtures/${prg}.ast.json tests/ast/${prg}.prg
done
rm -f fixture_*.c tests/ast/fixture_*.c
```

The compiler writes intermediate `fixture_*.c` artifacts; delete them before committing (and ensure `tests/ast/*.c` is not staged).

### Compile-buffer usage

In-process consumers call `hb_compMainExtModule()` with trace enabled. See `tests/ast/ast_compilebuf_tests.c` for a reference harness that captures token/node buffers from compile-buffer inputs under default and `-m` single-module flows.

### Tests to reuse

- `tests/ast/ast_trace_tests.c` — toggles trace enable/disable, validates token/boundary/node metadata, exercises diagnostics counters, and checks macro ancestry in JSON dumps.
- `tests/ast/ast_compilebuf_tests.c` — verifies compile-buffer callbacks (`hb_compMainExtModule`) across canonical fixtures.
- `tests/ast/fixture_inline_real.prg` — real-world INLINE/INIT/EXIT scenario; `ast_compilebuf_tests.c` consumes it to assert INLINE node emission and INIT/EXIT coverage without synthesised snippets.
- `tests/ast/ast_hbmk_ast_tests.c` — runs the compiler CLI with `--ast-trace-dump=-` and compares the payload against fixtures in `tests/ast/fixtures/`.
- `tests/ast/ast_preprocessor_trace_test.c` — drives `hb_pp_setTraceCallback()` directly, ensuring `.trace.json` and `.ppo` fixtures stay in sync.
- `tests/ast/ast_hbmk2_fixtures_test.c` — enforces that every `.prg` under `tests/ast/` compiles warning-free with `hbmk2 -w3`.

Rebuild and run the full suite with:

```sh
scripts/test-ast.sh
```

### Working with the trace

Consumers should call the `hb_compAstTrace*` accessors exported in `include/hbasttrace.h`:

- `hb_compAstTraceTokenCount()`, `hb_compAstTraceToken()` — enumerate emitted tokens (module, lexeme, PP type, offsets, retained `HB_PP_TRACEINFO`).
- `hb_compAstTraceBoundaryCount()`, `hb_compAstTraceBoundary()` — inspect parser boundary markers.
- `hb_compAstTraceNodeCount()`, `hb_compAstTraceNode()` — walk node enter/leave events (kind, id, phase, token reference, optional name).
- `hb_compAstTracePpEventCount()`, `hb_compAstTracePpEvent()` — read the preprocessor event log (rule kind, call site, expansion ancestry, source/result).
- `hb_compAstTraceDumpJson()` — emit the canonical JSON for downstream tools without spawning a subprocess.

Whenever you retain `HB_PP_TRACEINFO` pointers beyond the compiler’s lifetime, balance `hb_compAstTraceRetainInfo()` / `hb_compAstTraceReleaseInfo()` so reference counts remain correct. Diagnostics mode (`--ast-trace-diagnostics`) will surface leaks if counts diverge.

Node event `kind` values now distinguish compiler-init and exit routines (`FUNCTION_INIT`, `FUNCTION_EXIT`) and inline definitions (`INLINE`) in addition to the existing `FUNCTION`, class, statement, and expression buckets. Downstream consumers should treat the new kinds as peers to the original function entries.

### Why no standalone tooling?

The first attempt duplicated the preprocessor and serializer under `src/ast/lexer/` and `utils/hbast`. That code drifted from true compiler semantics and multiplied maintenance. The replacement wraps Harbour’s pipeline directly, so we:

- Emit trace data from `hb_comp_yylex`, parser reductions in `harbour.y`/`hbmain.c`, and the PP callback.
- Keep fixtures, cmocka coverage, and CLI usage anchored to the compiler executable.
- Avoid building or linking a separate `libhbastlex`/`hbast` binary — everything flows through `harbour`.

### Retired modules (2025-10-25)

Do **not** reintroduce the following first-attempt artefacts:

- `src/ast/` (parallel lexer/builder sources)
- `utils/hbast` and `utils/hbrename`
- Legacy hbast fixtures, cmocka suites, and associated documentation

Any experiments should live out of tree and consume `--ast-trace` / `--ast-trace-dump` output or the `hb_compAstTrace*` APIs directly.

### Future work

- Optional alternative encoders (e.g. CBOR) layered on top of the compiler-produced stream
- Track refactoring prototypes (`tests/ast/python/ast_refactor_cli.py`) and associated data-contract notes in `doc/agents/ast/lsp-refactoring-cli.md`; use `tests/ast/python/apply_workspace_edit.py` when replaying WorkspaceEdit payloads onto Harbour sources.
