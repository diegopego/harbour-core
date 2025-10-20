# AST Oversight and Delegation Briefs scratchpad

## Current Branch Snapshot (2025-10-25)
- HEAD `4ae9ecd262` builds on the pivot at `4da14975e24335cf02db6e340d4b0ec2e8ae75ce`, which removed the first-attempt lexer stack introduced in `d29cad47f5f8025136caa89f5a92392d13d87751`. All AST tooling now rides the compiler trace adapter.
- `hbtraceast.c` publishes tokens, lexer boundaries, node enter/leave events, and preprocessor traces with macro ancestry plus diagnostics counters. CLI toggles `--ast-trace`/`--no-ast-trace` and `--ast-trace-diagnostics`/`--no-ast-trace-diagnostics` are exercised in `tests/ast/ast_trace_tests.c`.
- Compile-buffer consumers (`hb_compMainExtModule`) receive the same stream; `tests/ast/ast_compilebuf_tests.c` covers default and `-m` single-module flows using canonical fixtures.
- `hb_compAstTraceDumpJson()` keeps JSON dumps aligned with `tests/ast/fixtures/*.ast.json`; CLI parity is enforced by `tests/ast/ast_hbmk_ast_tests.c`.
- Preprocessor trace fixtures (`tests/ast/preprocessor/fixtures/`) are verified through `tests/ast/ast_preprocessor_trace_test.c`, preserving `.trace.json` + `.ppo` artefacts.
- `tests/ast/ast_hbmk2_fixtures_test.c` guarantees every `.prg` under `tests/ast/` compiles warning-free via `hbmk2 -w3`. `scripts/test-ast.sh` rebuilds pp/compiler/test targets and runs the cmocka harness end-to-end.

## Agent Status Board

### Overseer
- **Current**: Docs-only edits on top of `ast-3rd-experiment`; instrumentation/tooling stack consolidated on compiler trace. No outstanding merges; working tree limited to coordination docs.
- **Next**: Maintain roadmap alignment, capture decisions and suggested commit messages in `doc/agents/ast/progress.md`, and ensure implementation sessions attach cmocka/hbmk2 evidence before closure.

### Compiler Instrumentation Agent
- **Current**: Token/boundary/node/PP emitters live across `complex.c`, `harbour.y`, and `hbmain.c`. Diagnostics counters + CLI toggles validated. Compile-buffer callbacks proven by `tests/ast/ast_compilebuf_tests.c`.
- **Next**: Extend node coverage for `INLINE`/`INIT`/`EXIT`, tag macro-generated statements, and close the `fSingleModule` audit. Update instrumentation docs when payloads or hook points evolve.
- [x] Emit node enter/leave events for class declarations (`DECLARE CLASS`, `DECLARE MEMBER`).
- [x] Capture statement-level reductions (IF/ELSE, FOR/NEXT, WHILE/ENDDO) with parent-child references.
- [x] Track codeblock (`{|| ... }`) entry/exit, linking to originating tokens.
- [ ] Surface inline (`INLINE`) definitions and `INIT/EXIT` procedures with dedicated node kinds.
- [ ] Map macro-generated constructs to node events once traceinfo-to-node bindings are stabilised.
- [x] Introduce optional debug counters (token/node totals, retained traceinfo) toggled via instrumentation flag for deeper diagnostics.
- [x] Build `hb_compileBuf` integration tests that compile canonical fixtures and compare emitted AST/token streams against golden snapshots.
- [ ] Audit the `fSingleModule` fast-path: confirm no module-level node events are lost when `hb_compParserRun()` skips reductions, and document mitigation if gaps remain.

### AST Tooling Agent
- **Current**: Golden dumps mirror compiler output; `README-AST.md`/`serialization-format.md` document the payload; preprocessor fixtures tracked.
- **Next**: Provide a consumer note or adapter stub for downstream tools, sync docs with future schema changes, and partner with the LSP agent on API expectations.

### Testing & Verification Agent
- **Current**: cmocka suites cover instrumentation toggles, diagnostics counters, compile-buffer flows, CLI snapshots, and PP fixtures. hbmk2 sweep enforces warning-free `.prg` files.
- **Next**: Add coverage once `INLINE`/`INIT`/`EXIT` events exist, watch PP traceinfo retain/release balances, and consider property-based checks for macro ancestry depth. Continue documenting regen steps when fixtures move.

### LSP & Refactoring Agent
- **Current**: CLI prototype (`scripts/ast_refactor_cli.py`), workspace edit applier (`tests/python/apply_workspace_edit.py`), pytest smoke coverage (`tests/python/test_refactor_cli.py`), and persistent fixtures (`tests/ast/fixture_demo.rename.prg`, `tests/ast/fixture_demo.extract.prg`) emit, materialise, and validate rename/extract `WorkspaceEdit` payloads derived from `hb_compAstTraceDumpJson()` dumps; VS Code contract notes captured in `doc/agents/ast/lsp-refactoring-cli.md`.
- **Next**: Harden rename scope detection (token-type filters, node ancestry), expand fixture coverage beyond `fixture_demo`, and coordinate schema assurances (macro expansions, INLINE/INIT/EXIT coverage) with the AST tooling agent.

### Open Oversight Follow-ups
- Coordinate with downstream consumers so they adopt the compiler dump schema; ensure no documentation references the retired `src/ast/` stack.

## Delegation Brief – Overseer Agent (Template)
Role: Overseer Agent  
Goal: Coordinate compiler/tooling work, ensure oversight docs stay current, and keep all fixtures/tests green (`hbmk2 -w3`, cmocka, `scripts/test-ast.sh`).  
Instructions: Review this draft plus `progress.md`, confirm repository status, then use the checklist below to steer implementation agents.

### Recurring Checklist
1. Audit repository state (`git status -sb`, divergence from `origin/ast-3rd-experiment`).
2. Review latest session reports; confirm objectives and test artefacts are logged in `doc/agents/ast/progress.md`.
3. Update this draft and `progress.md` with new decisions, follow-ups, or schedule changes.
4. Validate the verification matrix whenever fixtures/docs/tests move.
5. Refresh delegation briefs if scope or priorities shift.

### Session Workflow
**Start**: Gather current status (progress log, instrumentation plan), note outstanding risks, and identify sessions to spawn or continue.  
**During**: Coordinate agents, clarify acceptance criteria, approve or defer work items, and keep oversight docs tidy.  
**Before hand-off**: Summarise decisions, queue follow-ups, list next sessions (with prompts), and ensure `hbmk2 -w3` ran after any oversight-driven fixture changes.

### Reporting Template
State planning decisions (approvals/deferrals), sessions to launch next (roles/objectives), verification outcomes if oversight touched fixtures, and risks needing escalation.

## Delegation Brief – Compiler Instrumentation Agent (Template)
Role: Compiler Instrumentation Agent  
Goal: Advance compiler instrumentation per `doc/agents/ast/instrumentation-plan.md`. Ensure every `.prg/.ch/.hbm/.hbmk` fixture compiles warning-free with `hbmk2 -w3`; compiler-generated traces (CLI dump, compile-buffer) remain the golden reference.  
Instructions: 
- Follow Active Objectives and the Execution Checklist. Document progress/tests in `doc/agents/ast/progress.md` and this draft before hand-off.
### Active Objectives
- Extend diagnostics/telemetry only through documented compiler flags (`--ast-trace[-diagnostics]`) or environment toggles.
- Update `instrumentation-plan.md`, `hb_compilebuf_evaluation.md`, and serialization docs whenever payloads or toggles change.



### Execution Checklist
**Start**  
1. Read latest entries in `doc/agents/ast/progress.md`.  
2. Confirm `git status`.  
3. Note which objective(s) you plan to tackle and obtain Overseer approval if scope changes.  
4. Tooling on hand: `jq`/`jsonlint` for JSON inspection, `tree-sitter-cli` for grammar experiments (use only when justified).  
5. Keep terminology aligned with Harbour/CA‑Clipper references (`doc/references/c53g01c.txt`).

**During**  
- Update docs/fixtures alongside code changes.  
- Run targeted tests after each major edit (relevant cmocka targets, `scripts/test-ast.sh`, hbmk2 sweeps).

**Before hand-off**  
1. Run `make -C tests/ast tests` (or specific targets touched) plus `hbmk2 -w3` for affected `.prg` files.  
2. Log results and completed work in `doc/agents/ast/progress.md`.  
3. Refresh this draft (milestones, objectives, follow-ups).

## Verification Matrix
- **Tooling harness**:
  - `tests/ast/ast_trace_tests.c`: instrumentation toggles, node coverage, macro ancestry, diagnostics counters.
  - `tests/ast/ast_compilebuf_tests.c`: compile-buffer coverage for default and `-m` flows.
  - `tests/ast/ast_hbmk_ast_tests.c`: CLI dumps vs fixtures.
  - `tests/ast/ast_preprocessor_trace_test.c`: PP callback output and `.ppo` regeneration.
  - `tests/ast/ast_hbmk2_fixtures_test.c`: `hbmk2 -w3` sweep across fixture sets.
- **Run log**:
  - Generate traces via `harbour -iinclude --ast-trace --ast-trace-dump` or `hb_compAstTraceDumpJson()`; validate compile-buffer pathways with the cmocka harness.
- **Pass/Fail criteria**:
  - Token IDs from `hb_comp_yylex` reconcile with parser node events (no orphan tokens).
  - `HB_PP_TRACEINFO` retain/release counts net to zero per test run.
  - CA‑Clipper terminology stays consistent across docs and output.
- **Reporting**:
  - Log matrix status in `doc/agents/ast/progress.md` for sessions touching instrumentation.
  - Implementation agents attach test summaries to their reports.
