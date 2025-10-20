## AST instrumentation quick reference

Harbour’s AST tooling now rides entirely on the compiler itself. There is no separate lexer, builder, or utility binary to maintain — the single source of truth is the instrumentation exposed behind `--ast-trace`.

### Building once

```sh
make HB_BUILD_PARTS=compiler HB_PLATFORM=linux HB_COMPILER=gcc
```

This produces the compiler (`bin/<platform>/<compiler>/harbour`) with the AST trace hooks enabled.

### Capturing a trace

Run the compiler with the existing switches:

```sh
bin/linux/gcc/harbour -iinclude --ast-trace --ast-trace-dump=- tests/ast/fixture_demo.prg
```

This streams a JSON event log containing the token stream, boundary events, parser node enter/leave events, and preprocessor trace information. Redirect `--ast-trace-dump` to a file when you want to archive the snapshot.

### Refreshing trace fixtures

Regenerate the golden dumps that power `tests/ast/hbmk-ast-tests.c` with a single loop:

```sh
for prg in fixture_demo fixture_blocks fixture_ppdirectives fixture_statements \
           fixture_expressions fixture_includes fixture_compat_clipper fixture_compat_harbour \
           fixture_macro_expansion; do
  bin/linux/gcc/harbour -iinclude --ast-trace \
    --ast-trace-dump=tests/ast/fixtures/${prg}.ast.json tests/ast/${prg}.prg
done
rm -f fixture_*.c
```

The compiler leaves intermediate `fixture_*.c` sources in the working directory; remove them (and any staged `tests/ast/*.c`) before committing.

### Tests to reuse

- `tests/ast/ast_trace_tests.c` — drives `hb_comp_new()`/`hb_compAstTraceSetEnabled()` and asserts token, boundary, and PP event metadata.
- `tests/ast/ast_compilebuf_tests.c` — feeds in-memory source through `hb_compMainExtModule()` to prove compile-buffer workflows receive the same trace stream.
- `tests/ast/ast_hbmk_ast_tests.c` — invokes the compiler CLI with `--ast-trace-dump=-` and compares the JSON payload against fixtures under `tests/ast/fixtures/`.

Run everything with:

```sh
scripts/test-ast.sh
```

### Working with the trace

Consumers should call the `hb_compAstTrace*` accessors exposed in `include/hbasttrace.h`:

- `hb_compAstTraceTokenCount()`, `hb_compAstTraceToken()` — enumerate emitted tokens (module, lexeme, PP type, offsets, `HB_PP_TRACEINFO` pointer).
- `hb_compAstTraceBoundaryCount()`, `hb_compAstTraceBoundary()` — inspect parser boundary markers.
- `hb_compAstTracePpEventCount()`, `hb_compAstTracePpEvent()` — walk the preprocessor event log (macro name, call module/range, expansion id, source/result).
- `hb_compAstTraceDumpJson()` — obtain the canonical JSON without launching a subprocess.

When you retain `HB_PP_TRACEINFO` pointers for longer than the compiler lifetime, make sure to balance `hb_compAstTraceRetainInfo()`/`hb_compAstTraceReleaseInfo()` to keep reference counts in sync.

### Why no standalone tooling?

The earlier “first attempt” duplicated the preprocessor and serializer under `src/ast/lexer/` and `utils/hbast`. That design drifted from the true compiler semantics and multiplied maintenance. The second attempt wraps Harbour’s own pipeline, so we now:

- Emit all trace data directly from `hb_comp_yylex`, `harbour.y`, and the PP callback.
- Keep fixtures, cmocka coverage, and CLI usage anchored to the compiler executable.
- Avoid building or linking a separate `libhbastlex` / `hbast` binary — everything flows through `harbour`.

Future work (macro trace normalisation, additional node metadata, CBOR export) should extend the compiler trace or its dump routine so the single source of truth remains inside `src/compiler/`. Use the existing tests as templates for new scenarios. !*** End Patch
