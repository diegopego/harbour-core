# AST Oversight and Delegation Briefs scratchpad

## Alignment Memo
- *Harbour core scope: `include/hbpp.h`, `src/pp/ppcore.c`, `src/compiler/*`, `src/harbour.def`, and the instrumentation docs (`Agents.md`, `doc/agents/ast/progress.md`, this draft, `doc/agents/ast/hb_compilebuf_evaluation.md`) remain in-tree. They jointly define, exercise, and document the compiler trace pipeline.
- Changes should maintain upstream Harbour compatibility and rely on compiler truth (`HB_PP_TRACEINFO`/parser hooks), and be backed by test/maintenance commitments.
- Commands: `harbour -iinclude --ast-trace --ast-trace-dump=<fixture>.ast.json tests/ast/<fixture>.prg`.

## Parser Hook Backlog
- [x] Emit node enter/leave events for class declarations (`DECLARE CLASS`, `DECLARE MEMBER`). (Landed 2025-10-20)
- [x] Capture statement-level reductions (IF/ELSE, FOR/NEXT, WHILE/ENDDO) with parent-child references. (Core coverage; continue auditing remaining rules)
- [x] Track codeblock (`{|| ... }`) entry/exit, linking to originating tokens.
- [ ] Surface inline (`INLINE`) definitions and `INIT/EXIT` procedures with dedicated node kinds.
- [ ] Map macro-generated constructs to node events once traceinfo-to-node bindings are stabilised.
- [ ] Introduce optional debug counters (token/node totals, retained traceinfo) toggled via instrumentation flag for deeper diagnostics.
- [ ] Build `hb_compileBuf` integration tests that compile canonical fixtures and compare emitted AST/token streams against golden snapshots. (See `doc/agents/ast/hb_compilebuf_evaluation.md` for callback/API strategy.)

## Delegation Brief – Overseer Agent (Template)
Role: Overseer Agent
Goal: Coordinate compiler/tooling work, ensure oversight docs stay current, and keep all fixtures/tests green (`hbmk2 -w3`, cmocka, `scripts/test-ast.sh`).
Instructions: read draft.md to familiarize, use the `### Recurring Checklist` below to audit progress, refresh documentation, and hand off cleanly to implementation agents.

### Overview
- Maintain roadmap alignment, repository hygiene, and risk management for AST instrumentation/tooling.
- Approve delegation scopes and verify that golden snapshots (compiler-derived) remain authoritative.
- Ensure documentation (`Agents.md`, `doc/agents/ast/*`) reflects the latest workflows.

### Inputs & References
- `doc/agents/ast/progress.md` – source of truth for recent work & test logs.
- `doc/agents/ast/instrumentation-plan.md`, `hb_compilebuf_evaluation.md`, and this draft – planning artifacts and backlogs.
- Verification matrix (above) – mandatory test gates.
- Upstream baseline status via `git status -sb`, `git log`, and `hbmk2` sweeps.

### Recurring Checklist
1. Audit repository state (`git status`, pending files, unmerged branches).
2. Review latest session reports; confirm objectives & tests align with the plan.
3. Update this draft and `progress.md` with new decisions, follow-ups, or schedule changes.
4. Validate the verification matrix if implementation work touched fixtures/docs.
5. Refresh delegation briefs if scope or priorities shift.

### Session Workflow
**Start**: gather current status (progress log, instrumentation plan), note outstanding risks, and identify which delegation sessions must be spawned/continued.

**During**: coordinate agents, clarify acceptance criteria, approve or defer work items, and keep oversight docs tidy (no stale instructions).

**Before hand-off**: summarise decisions, update outstanding follow-ups, list required next sessions (with prompts), and ensure `hbmk2 -w3` has been run after any oversight-driven fixture changes.

### Reporting Template
Overseer wrap-up should state: planning decisions made (approvals, deferrals), sessions to spawn next (with roles/objectives), verification outcomes if oversight touched fixtures, and any risks needing escalation.

### Open Oversight Follow-ups
- Coordinate with tooling migration to ensure external consumers (vscode/lsp) adopt the compiler dump schema.

## Delegation Brief – Compiler Instrumentation Agent (Template)
Role: Compiler Instrumentation Agent
Goal: Advance compiler instrumentation per doc/agents/ast/instrumentation-plan.md. Ensure every .prg/.ch/.hbm/.hbmk fixture compiles warning-free with hbmk2 -w3; compiler-generated traces (CLI dump, compile-buffer) are the golden reference.
Instructions: Follow Active Objectives and the Execution Checklist. Document progress/tests in doc/agents/ast/progress.md and this draft before hand-off.

### Overview
- **Mandate**: Keep Harbour’s compiler as the single source of truth by emitting token, boundary, node, and preprocessor events backed by `HB_PP_TRACEINFO` (see `doc/agents/ast/instrumentation-plan.md`). All `.prg`/`.ch`/`.hbm`/`.hbmk` fixtures must remain warning-free under `hbmk2 -w3`; compiler-produced traces (CLI dump, compile-buffer) are the golden reference.
- **Scope**: Manage PP trace lifecycle (`hb_pp_setTraceCallback`), instrument `hb_comp_yylex`, parser reductions (`harbour.y` / `hbmain.c`), feature toggles, and trace dump/compile-buffer paths.

### Inputs & References
- Instrumentation plan (`doc/agents/ast/instrumentation-plan.md`) – hook map, run log, open questions (golden snapshots, diagnostics counters).
- Progress log (`doc/agents/ast/progress.md`) – recent session reports/testing evidence.
- Compile-buffer evaluation (`doc/agents/ast/hb_compilebuf_evaluation.md`).
- Verification matrix (this draft) – hbmk2 sweeps, cmocka suites, `scripts/test-ast.sh`, snapshot rules.
- this branch starts at the commit d29cad47f5f8025136caa89f5a92392d13d87751. pay attention on the commit d29cad47f5f8025136caa89f5a92392d13d87751 where we took the decision to remove the first attempt lexer surfaces and introduce the trace adapter using only extensions on harbour core code.

### Achieved Objectives
- Lexer emission pass: Tokens/boundaries published from `hb_comp_yylex`.
- Parser hook pilot: Functions/classes/control-flow instrumented
- Verification sweep: tests/ast
- Single-module coverage: `hb_compParserRun` now routes through `hb_comp_yylex`; both compile-buffer and CLI harnesses iterate over `fixture_demo` and `fixture_blocks` under default/`-m` modes.
- Diagnostics toggles: CLI/env flag (`--ast-trace-diagnostics` / `HB_AST_TRACE_DIAGNOSTICS`) now exposes token/boundary/node/traceinfo totals for troubleshooting.  

### Active Objectives
- generate a small refactoring CLI in harbour to consume compiler-generated AST traces and perform simple transformations (e.g., rename a function, extract a method) so we can validate end-to-end tooling integration.
- Trace publication – the compiler should output the ast for external tools such as VSCODE/LSP to consume.  
- Docs & cleanup – update `instrumentation-plan.md`, `hb_compilebuf_evaluation.md`, CLI/env docs with the new toggles and fixture workflow.  

### Execution Checklist
**Start**  
1. Read latest entries in `doc/agents/ast/progress.md`.  
2. Confirm `git status`
3. Note which objective(s) you plan to tackle.  
4. Tooling on hand: `jq` / `jsonlint` for trace JSON inspection, `tree-sitter-cli` for quick grammar tests (use only when really needed). 
5. When naming new fixtures, tokens, or events, consult the CA‑Clipper 5.3 guide (`doc/references/c53g01c.txt`) and Harbour docs first so terminology stays aligned with the language’s canon.

**During**  
- Update docs/fixtures with related code changes.  
- Run targeted tests after each major edit.

**Before hand-off**  
1. Run tests/ast
2. Log results + completed work in `doc/agents/ast/progress.md`.  
3. Refresh this draft (milestones, objectives, follow-ups).

## Verification Matrix
- **Tooling harness**:
  - `tests/ast` cmocka suites: extend to validate that emitted token events and AST node events match expected shapes; include leak detectors for `HB_PP_TRACEINFO`.
  - `scripts/test-ast.sh`: rerun snapshot comparisons using compiler-sourced events; failures indicate divergence from the schema/fixtures.
- **Run log**:
  - trace-dump can be generated with hbmk2 (`--ast-trace`, `--ast-trace-dump`, `HB_AST_TRACE_DUMP`) or `hb_compileBuf` to produce the JSON streams.
- **Pass/Fail criteria**:
  - Token IDs emitted by `hb_comp_yylex` should reconcile with nodes collected in parser reductions (no orphan tokens/nodes).
  - `HB_PP_TRACEINFO` retain/release counts must net to zero after each test run (cmocka assertion).
  - Ensure CA‑Clipper terminology (PROC/STATIC/LOCAL) remains consistent when rewriting docs and CLI output.
- **Reporting**:
  - Log matrix status in `doc/agents/ast/progress.md` for each session touching instrumentation.
  - Implementation agents attach test output summaries to their session reports.



