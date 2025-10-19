# AST Tooling Progress Log

## 2025-10-22 – Verification Sweep (`hbmk2 -w3`)

- **Testing**: Ran `hbmk2 -w3` against `tests/ast/fixture_demo.prg`, `tests/ast/preprocessor/fixtures/macro_trace.prg`, and `tests/ast/preprocessor/fixtures/command_trace.prg`. After refactoring the fixture to functions and adding `CallIncludedProc()` to exercise the static helper, the sweep now completes without warnings.
- **Outcome**: Compiler instrumentation behaved as expected under strict warnings and the helper include is fully consumed. Fixtures are back to warning-free state for the verification matrix.
- **Automation**: Added a `cmocka` runner (`tests/ast/hbmk2-fixtures`) that globs all `.prg` fixtures, shells out to `hbmk2 -w3`, and fails the suite if warnings/errors appear; wired into `tests/ast/Makefile`.
- **Open items**: Re-run `hbmk2 -w3` when new fixtures land, and continue evaluating `hb_compileBuf` golden tests for broader coverage.

## 2025-10-21 – Compiler Instrumentation Hardening

- **Code updates**: Finalised lifecycle wiring so `hb_comp_new()`/`hb_comp_free()` install and tear down the PP trace callback, ensured `hb_comp_yylex` routes all returns through `hb_compAstTraceReturn()`, and exercised the expression/statement macros (`HB_AST_TRACE_EXPR`, stack helpers) with refreshed cmocka coverage (`expression_nodes_capture_reductions` fix).
- **Docs**: Updated the instrumentation plan with a status table and carried the oversight notes into the draft scratchpad; flagged the remaining single-module parser hook as pending.
- **Testing**: `scripts/test-ast.sh` passes with tracing toggled both on and off; outstanding suites limited to the planned `hbmk2 -w3` sweep.
- **Open items**: Schedule `hbmk2 -w3` over affected fixtures, decide whether single-module parsing needs additional buffering before widening tests, and continue the expression-reduction backlog for remaining grammar cases.

## 2025-10-20 – PP Trace Sink & Toggle

- **Code updates**: Implemented `hb_compAstTracePublishPreprocessorEvent()` and storage for preprocessor trace payloads (`include/hbasttrace.h`, `src/compiler/hbtraceast.c`), exposing accessor helpers and clearing logic so traceinfo retain/release stays balanced. Added `--ast-trace` / `--no-ast-trace` CLI switches plus `HB_AST_TRACE` environment override in `src/compiler/cmdcheck.c`, propagating toggles through `hb_compChkParseSwitch()` and wiring new cmocka coverage (`cli_toggle_controls_trace`, `pp_events_capture_macro_traces`) in `tests/ast/ast_trace_tests.c`.
- **Instrumentation behaviour**: Preprocessor callbacks now duplicate rule/macro/source/result strings, retain `HB_PP_TRACEINFO`, and sequence events alongside tokens/boundaries for deterministic replay via `hb_compAstTracePpEvent{Count,}`. CLI/environment toggles call `hb_compAstTraceSetEnabled()` so instrumentation state is settled before compilation begins.
- **Testing**: `scripts/test-ast.sh` rebuilt the tree and ran the cmocka suites (`tests/ast/trace-events`, `pp-trace`, `builder-test`, etc.); all tests passed with instrumentation enabled and disabled toggles verified.
- **Open items**: Extend parser event coverage beyond function declarations, expose consumer bridge for buffered PP/AST streams, and schedule the pending `hbmk2 -w3` verification pass over fixtures.

## 2025-10-20 – Parser Hooks Extended

- **Code updates**: Added new node kinds (`CLASS`, `CLASS_METHOD`, `CLASS_DATA`, `STATEMENT_*`, `CODEBLOCK`) and stack helpers to `hbtraceast.c`, letting grammar actions publish nested statement and block events without requiring handles. Instrumented `harbour.y` to emit enter/leave events for class declarations/members, control-flow statements (IF/CASE/WHILE/FOR/FOREACH/SWITCH/WITH/BEND SEQUENCE), and codeblock literals. Updated trace tests with coverage for the new node kinds (`class_and_member_events_capture_names`, `statement_stack_records_nested_nodes`).
- **Behavioural notes**: Statement instrumentation uses the stack helpers so nested constructs unwind correctly; leave events reuse the latest token ID while class/member nodes duplicate symbol names for correlation. Codeblock enter/leave now bracket both inline and extended `{|| ... }` forms.
- **Testing**: `scripts/test-ast.sh` (cmocka suites + shell harness) passes with the expanded test matrix; new tests sit under `tests/ast/trace-events` ensuring name propagation and stack ordering. Full `hbmk2 -w3` run still pending.
- **Open items**: Remaining expression coverage (binary reductions, macro expressions) and error-path flushing still outstanding; need to surface node IDs through consumer API once downstream tooling is ready.

## 2025-10-19 – Trace Callback Foundation

- **Code updates**: Instrumentation scaffold added (`include/hbasttrace.h`, `src/compiler/hbtraceast.c`, `include/hbcompdf.h`, `src/compiler/hbcomp.c`, `src/compiler/Makefile`) and preprocessor helpers exported (`include/hbpp.h`, `src/pp/ppcore.c`, `src/harbour.def`) to make `hb_pp_traceinfoRetain/Release` available to the compiler.
- **Lifecycle wiring**: `hb_comp_new()` now delegates to `hb_compAstTraceInit()` and `hb_comp_free()` shuts down the callback path, detaching `hb_pp_setTraceCallback()` so instrumentation remains opt-in via `hb_compAstTraceSetEnabled()`.
- **Testing**: Added a cmocka stress test (`tests/ast/ast_trace_tests.c`) that exercises retain/release accounting; build runner not executed in this session (pending consolidated instrumentation harness).
- **Open items**: Provide real event sinks in `hb_compAstTracePpSink`, surface a user-facing toggle (CLI/env) for enabling tracing, and integrate the cmocka target into the verification matrix runs.

## 2025-10-19 – Lexer Emission Pass

- **Code updates**: Instrumented `hb_comp_yylex` via `hb_compAstTracePublishToken/Boundary`, replaced raw returns with a guardable helper, and expanded `hbtraceast.c` to keep token/boundary queues with stable IDs and retained trace handles.
- **Event schema**: `include/hbasttrace.h` now exposes `HB_COMP_AST_TRACE_TOKEN` / `_BOUNDARY` structures; documentation updated to describe payload fields and sequencing.
- **Testing**: Added cmocka coverage (`lexer_respects_disabled_state`, `lexer_emits_tokens`) that initialises the real preprocessor, validates the disabled path, and asserts token/boundary capture. Test runner still pending integration (not executed this session) pending build wiring.
- **Open items**: Flush buffers on feature toggle transitions for long-lived compilers, expose snapshot/export helpers for tooling consumers, and wire hbmk2/cmocka targets into the verification matrix.

## 2025-10-19 – Parser Hook Pilot

- **Code updates**: `harbour.y` now calls `hb_compAstTraceNodeEnter()` on function reductions, while `hb_compFinalizeFunction()` emits the matching leave via `hb_compAstTraceNodeLeave()`; `hbtraceast.c` gained node queues, stable IDs, and mapper bookkeeping.
- **Data model**: Defined `HB_COMP_AST_TRACE_NODE_EVENT` (`id`, `sequence`, `kind`, `phase`, `tokenId`, `name`, `handle`) with mapping to `HB_HFUNC` handles so leave events reuse entry IDs.
- **Testing**: Added cmocka coverage (`parser_emits_function_nodes`) in `tests/ast/ast_trace_tests.c` that runs the full parser/compile-end path to assert enter/leave pairs. Suite still not executed in-session (build integration pending).
- **Open items**: Implement node publishers for class/method declarations, attach statement tree nodes (IF/LOOP/etc.), and extend tests to assert parent-child relationships once hooks land.

## 2025-10-18 – Oversight Baseline Review

- **Repository status**: `git status -sb` shows local edits to `Agents.md` (roles rewrite) and an existing modification in `draft.md`. No other worktree changes detected; outstanding content in `draft.md` must be preserved when committing.
- **Compiler token/AST flow snapshot**:
  - `hbmain.c` initialises the runtime, loads compiler configuration, and hands execution to `hb_compMain()` in `hbcomp.c`.
  - `hbcomp.c` orchestrates command parsing, preprocessor setup, and compilation units; it drives the scanner via `hb_compParserDo()`, wiring the lexer (`hb_pp_lex*` routines in `complex.c`) to the parser.
  - `complex.c` owns the token pipeline: it integrates the Harbour preprocessor, produces `HB_PP_TOKEN` records, tracks source modules, and feeds tokens to the parser while managing include stacks and macro expansion.
  - `harbour.y` defines the grammar and semantic actions that build `PHB_EXPR` trees; its generated counterpart `harbour.yyc` contains the C implementation (`hb_yyparse`, reduction handlers) consumed at build time.
  - Semantic data is layered through `ppcore.c`/`function.c` helpers that attach scope and symbol metadata to `PHB_EXPR` nodes before code generation.
- **New branch additions (ast-3rd-experiment)**:
  - `src/ast/lexer/hbast_lexer.c` implements an incremental lexer that mirrors the preprocessor, capturing macro traces (`HB_PP_TRACEINFO`), source ranges, and token history snapshots.
  - `src/ast/lexer/hbast_builder.c` provides a standalone builder to assemble node and symbol graphs, backed by dynamic arrays and JSON/CBOR serialization helpers in `src/ast/lexer/hbast_json.c`.
  - Tooling docs under `doc/agents/ast/` describe serialization schemas, incremental lexer design, and verification strategy (`hbast-verify.md`, `incremental-lexer.md`, etc.).
  - Tests in `tests/ast/` exercise the prototype lexer/builder via cmocka suites and snapshot fixtures, plus shell helpers in `scripts/test-ast.sh`.
- **Gaps between compiler AST and tooling AST**:
  - Tooling currently replays preprocessor output independently; the compiler proper (`complex.c`, `harbour.y`) is not yet instrumented to emit the same token stream, causing duplication and potential drift (ranges, macro depth, trivia channels).
  - `PHB_EXPR` trees built during parsing are not exposed to the tooling builder; the JSON/CBOR output is generated from a parallel structure that lacks guaranteed parity with compiler semantics (symbol resolution, codegen flags, hidden nodes).
  - Macro trace fidelity depends on reconstructing `HB_PP_TRACEINFO` snapshots outside the compiler, with no guarantees that parser-time transformations (e.g., implicit statements, aliasing) are reflected.
  - No current bridge exists from the compiler’s semantic passes (scope resolution, optimisations) to the tooling layer, leaving refactoring features blind to compiler-only insights.
- **Immediate follow-ups**:
  - Finalise instrumentation plan for hooking `complex.c` token lifecycle and `harbour.y` reductions without regressing compiler behaviour.
  - Align tooling builder schemas with real `PHB_EXPR` node kinds to avoid divergence as grammar evolves.
  - Document outstanding change in `draft.md` before committing to keep historical context intact.

## 2025-10-18 – Oversight Instrumentation Kickoff

- **Scope survey**: Reviewed divergence against `cfb7bdc22c3bb722ddecc3b6c1c1a310e03a66ca`; branch introduces macro trace plumbing in `src/pp/ppcore.c` and standalone tooling in `src/ast/lexer/`.
- **State of instrumentation**: Compiler remains uninvolved with the tooling prototypes—`HB_PP_TRACEINFO` data is extended but not yet streamed to consumers, and `PHB_EXPR` trees stay private to parser actions.
- **Prioritised actions**:
  - Author `doc/agents/ast/instrumentation-plan.md` capturing concrete hook points (token emission in `complex.c`, reduction callbacks in `harbour.y`, aggregation in `hbcomp.c`).
  - Prepare delegation brief for the Compiler Instrumentation Agent to surface stable token IDs and macro traces from the real preprocessor into the AST pipeline.
  - Prepare delegation brief for the AST Tooling Agent to map `PHB_EXPR` kinds onto the JSON/CBOR schema, identifying missing coverage in `hbast_builder`.
- **Risks & guards**: `ppcore.c` modifications touch critical preprocessor paths; mandate `hbmk2 -w3` and cmocka suites before any commit integrating new hooks.
- **Follow-ups**: Update `doc/agents/ast/draft.md` with delegation outlines and testing matrix revisions ahead of implementation sessions.

## 2025-10-18 – Phase 0 Branch Assessment Kickoff

- **Mandate**: Before pushing instrumentation work, determine whether `ast-3rd-experiment` should be retained as-is, split, or rewritten to align with Harbour core goals.
- **Branch divergence snapshot**:
  - Core touchpoints: `.gitignore`, top-level `Makefile`, `include/Makefile`, `include/hbpp.h`, `src/Makefile`, `src/harbour.def`, `src/pp/ppcore.c`, `utils/Makefile`.
  - Tooling overlay: new docs, schemas, `src/ast/` subsystem, CLI utilities (`utils/hbast`, `utils/hbrename`), dedicated tests/fixtures under `tests/ast/` and cmocka harness.
- **Evaluation tracks**:
  1. Validate whether core changes (preprocessor trace plumbing, build wiring) are production-ready or should be side-loaded via optional modules.
  2. Decide which tooling assets live outside the Harbour core tree (e.g., separate repo/tooling pack) vs what must integrate tightly.
  3. Identify any prototype code that should be dropped or rewritten before alignment.
- **Next actions**:
  - Catalogue each divergent file with rationale (keep, isolate, drop) and dependencies.
  - Draft alignment proposal documenting boundary between core compiler and external tooling deliverables.
  - Use the proposal to drive subsequent delegation briefs for implementation agents.
  - Documented governance rule in `Agents.md` requiring Overseer sessions to only suggest commit messages (no direct commits).
  - Divergence ledger captured in `doc/agents/ast/divergence-ledger.md`.
  - Decision criteria recorded; alignment memo now includes timeline and sequencing milestones (2025-10-20 core fixes, 2025-10-27 tooling extraction, 2025-11-03 instrumentation restart) in `doc/agents/ast/draft.md`.
  - Instrumentation plan with hook mapping and migration strategy published in `doc/agents/ast/instrumentation-plan.md`.
  - Verification matrix outlining mandatory suites/fixtures/pass-fail criteria documented in `doc/agents/ast/draft.md`.
  - Delegation task packets (scope, incremental breakdown, verification requirements) for Compiler Instrumentation Agent and AST Tooling Agent documented in `doc/agents/ast/draft.md`.
## 2025-10-18 – Compiler Instrumentation Session Report

- **Scope covered**: Steps 1–3 of the Compiler Instrumentation brief landed. `hb_comp_new()`/`hb_comp_free()` now route through `hb_compAstTraceInit/Done`, wiring `hb_pp_setTraceCallback()` and managing `HB_PP_TRACEINFO` refcounts. Token events flow from `hb_comp_yylex()` via `hb_compAstTracePublishToken()`/`hb_compAstTraceReturn()`, and `harbour.y` function productions emit enter/leave node events through `hb_compAstTraceNode{Enter,Leave}()`.
- **Artifacts**: New helper module `hbtraceast.c`, guarded state in `hbcompdf.h`, retained/released APIs in `include/hbpp.h`, and updated docs (`doc/agents/ast/instrumentation-plan.md`, `doc/agents/ast/draft.md`), including a parser hook backlog.
- **Verification status**: `tests/ast/ast_trace_tests.c`, existing cmocka suites, and `scripts/test-ast.sh` (with the new `trace-events` binary) pass; instrumentation disabled mode covered. `hbmk2 -w3` over fixtures still pending before merge.
- **Open follow-ups**:
  - Implement real event sink in `hb_compAstTracePpSink` and define consumer API.
  - Extend parser hooks beyond function declarations (classes, statements, expressions) per backlog.
  - Add CLI/env toggle to expose instrumentation flag.
  - Run `hbmk2 -w3` on relevant fixtures to satisfy verification matrix.
