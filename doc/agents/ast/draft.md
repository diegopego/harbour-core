# Working scratchpad for notes and decisions

## 2025-10-18 Oversight Baseline Review
- Rewrote `Agents.md` to define oversight, implementation roles, and roadmap guardrails for the refactoring tooling mission.
- Logged current compiler vs tooling architecture alignment in `doc/agents/ast/progress.md`.
- Outstanding: draft instrumentation plan for compiler-integrated token/AST events and reconcile local `draft.md` edits prior to baseline commit.

## Follow-ups (Oversight Instrumentation Kickoff)
- DONE 2025-10-18: `doc/agents/ast/instrumentation-plan.md` documents hooks in `complex.c`, `harbour.y`, `hbcomp.c`, plus data contracts to tooling.
- DONE 2025-10-18: Delegation briefs prepared for Compiler Instrumentation Agent and AST Tooling Agent (see dedicated sections below).
- Define verification matrix for future commits (token parity fixtures, `PHB_EXPR` node coverage) before authorising implementation sessions.
- Track post-session follow-ups: expanded parser hooks and `hbmk2 -w3` sweeps (event sink + instrumentation toggle landed 2025-10-20).
- 2025-10-22: Added `tests/ast/hbmk2-fixtures` cmocka runner to enforce `hbmk2 -w3` cleanliness for all `.prg` fixtures.
- Drafted `doc/agents/ast/hb_compilebuf_evaluation.md` outlining the callback/API work needed to harvest instrumentation outputs from `hb_compileBuf`.
- Evaluate adding `hb_compileBuf`-based fixtures as golden AST references so only Harbour-compilable `.prg`/`.ch` inputs drive instrumentation tests.
- Consider lightweight debug counters inside trace helpers to diagnose event sequencing (token/node counts, retained traceinfo) when future sessions debug hooks.
- 2025-10-22: `hbmk2 -w3` sweep recorded; fixture tweaks (function conversions + `CallIncludedProc()`) cleared prior warnings so the matrix is green again.
- 2025-10-23: CLI trace dump and compile-buffer harness landed; next instrumentation session should focus on golden snapshot comparisons, `hb_compParserRun` coverage, and diagnostics counters.

## Phase 0 Assessment TODOs
- DONE 2025-10-18: Divergence ledger recorded in `doc/agents/ast/divergence-ledger.md` (`keep / isolate / drop` vs `cfb7bdc22c3bb722ddecc3b6c1c1a310e03a66ca`).
- Document decision criteria: impact on upstream parity, necessity for compiler-backed refactorings, maintenance cost if embedded in core.
- Draft an alignment memo summarising what stays in Harbour core, what moves to tooling distribution, and what is discarded, then update roadmap accordingly.
- Note in governance checklist: Overseer prepares commit messages only; user executes commits.

## Decision Criteria (Phase 0)
- **Upstream parity risk**: Keep changes in core only if they minimally impact merge effort against Harbour main; high-risk divergences favor isolation.
- **Compiler truth dependency**: Retain in core when instrumentation requires direct access to compiler internals (`HB_PP_TRACEINFO`, parser hooks); tooling-only logic moves out.
- **Maintenance burden**: Embed in core only when there is a team commitment to maintain the code long-term; prototypes with uncertain ownership belong in tooling repos.
- **Testing guarantees**: Core-integrated code must be covered by Harbour’s existing test suites; isolated tooling can rely on dedicated harnesses.
- **Packaging boundaries**: Utilities or docs aimed at external workflows should live in the tooling distribution, not the compiler tree.

## Verification Matrix
- **Core compiler gates**:
  - `hbmk2 -w3` on all `.prg` fixtures touched by instrumentation to ensure no new warnings/errors surface.
  - Harbour compiler self-tests covering macro-heavy code paths where traces are emitted.
- **Tooling harness**:
  - `tests/ast` cmocka suites: extend to validate that emitted token events and AST node events match expected shapes; include leak detectors for `HB_PP_TRACEINFO`.
  - `scripts/test-ast.sh`: rerun snapshot comparisons using compiler-sourced events; failures indicate divergence from the schema/fixtures.
- **Run log**:
  - 2025-10-22: `hbmk2 -w3` run logged; fixtures now compile cleanly after exercising the static helper include, the new `hbmk2-fixtures` cmocka target automates the check, the `hb_compMainExt()` finish callback is exercised for instrumentation snapshotting, `hb_compMainExtModule()` enables virtual module names for buffer compilations, the compiler can dump instrumentation via `--ast-trace-dump` / `HB_AST_TRACE_DUMP`, the `tests/ast/compilebuf-tests` harness validates token emission using that name while retaining the generated `tests/ast/compilebuf_fixture.c`, `tests/ast/hbmk-ast-tests` compares CLI-generated JSON against fixtures under `tests/ast/fixtures`, and `doc/agents/ast/hb_compilebuf_evaluation.md` captures the plan for golden tests driven by `hb_compileBuf`.
- **Fixtures & snapshots**:
  - Reuse existing `tests/ast/fixture_*.prg`, `.ch`, `.json`, `.ppo`, `.trace.json` files; add new variants for nested macros, conditionals, and dialect switches once instrumentation lands.
  - Maintain golden snapshots for both token streams and AST payloads; store under extracted tooling repo but reference versions in Harbour core for parity checks.
- **Pass/Fail criteria**:
  - All suites above must pass without warnings; token and AST snapshots must hash-match the golden copies.
  - Token IDs emitted by `hb_comp_yylex` should reconcile with nodes collected in parser reductions (no orphan tokens/nodes).
  - `HB_PP_TRACEINFO` retain/release counts must net to zero after each test run (cmocka assertion).
  - Compiler regression tests must show no change in output unless an instrumentation feature flag is enabled.
- **Reporting**:
  - Log matrix status in `doc/agents/ast/progress.md` for each session touching instrumentation.
  - Implementation agents attach test output summaries to their session reports.

## Parser Hook Backlog
- [x] Emit node enter/leave events for class declarations (`DECLARE CLASS`, `DECLARE MEMBER`). (Landed 2025-10-20)
- [x] Capture statement-level reductions (IF/ELSE, FOR/NEXT, WHILE/ENDDO) with parent-child references. (Core coverage; continue auditing remaining rules)
- [x] Track codeblock (`{|| ... }`) entry/exit, linking to originating tokens.
- [ ] Surface inline (`INLINE`) definitions and `INIT/EXIT` procedures with dedicated node kinds.
- [ ] Map macro-generated constructs to node events once traceinfo-to-node bindings are stabilised.
- [ ] Introduce optional debug counters (token/node totals, retained traceinfo) toggled via instrumentation flag for deeper diagnostics.
- [ ] Build `hb_compileBuf` integration tests that compile canonical fixtures and compare emitted AST/token streams against golden snapshots. (See `doc/agents/ast/hb_compilebuf_evaluation.md` for callback/API strategy.)

## Alignment Memo (Draft)
- **Stay in Harbour core**:
  - `include/hbpp.h`, `src/pp/ppcore.c`, `src/harbour.def`: foundational macro trace APIs required for any compiler-backed tooling.
  - Governance docs (`Agents.md`, `doc/agents/ast/progress.md`, `doc/agents/ast/draft.md`): shared oversight infrastructure.
  - Target timeline: retain and harden these APIs before resuming instrumentation work in Phase 2 (goal: ready for review by 2025-11-01).
- **Move to tooling distribution**:
  - Entire `src/ast/` subtree, AST headers under `include/ast/`, utilities in `utils/hbast`, `utils/hbrename`, `tests/ast/`, and associated scripts/docs (`README-AST.MD`, serialization specs, schema, fixtures).
  - Build glue (`Makefile` test target, `include/Makefile`, `src/Makefile`, `utils/Makefile`) that only serves the tooling prototype.
  - Extraction plan: split into a tooling package branch by 2025-11-08, preserving fixtures and scripts for reuse.
- **Drop / rewrite before reintegration**:
  - Prototype code flagged as “Isolate” without direct path to core instrumentation remains out-of-tree until rewritten against the final compiler APIs.
  - Re-evaluate after tooling extraction; schedule decision checkpoint for 2025-11-15 to determine which components re-enter Phase 3 (token event stream).
- **Sequencing for roadmap**:
  1. Week of 2025-10-20: apply minimal fixes to core trace APIs (`include/hbpp.h`, `src/pp/ppcore.c`) and document expectations in the instrumentation plan.
  2. Week of 2025-10-27: coordinate tooling extraction, ensuring build scripts/docs/tests move to the new tooling repo without impacting Harbour core.
 3. Week of 2025-11-03: revisit instrumentation plan and begin drafting implementation briefs based on the stabilized core. Fold in the CLI trace-dump flow (`--ast-trace-dump` / `HB_AST_TRACE_DUMP`), note outstanding polish (`--ast-trace-dump=-` stdout handling, trace dump cleanup), identify the next fixture additions for `tests/ast/hbmk-ast-tests`, and outline doc/test updates needed for the verification workflow.

## Delegation Brief – Compiler Instrumentation Agent (Template)

### Overview
- **Mandate**: Keep Harbour’s compiler as the single source of truth by emitting token, boundary, node, and preprocessor events backed by `HB_PP_TRACEINFO` (see `doc/agents/ast/instrumentation-plan.md`).
- **Scope**: Manage PP trace lifecycle (`hb_pp_setTraceCallback`), instrument `hb_comp_yylex`, parser reductions (`harbour.y` / `hbmain.c`), feature toggles, and trace dump/compile-buffer paths.

### Inputs & References
- Phase 0 decision criteria & parser-hook backlog (this draft).
- Instrumentation plan (`doc/agents/ast/instrumentation-plan.md`) – hook map, run log, open questions (golden snapshots, diagnostics counters).
- Progress log (`doc/agents/ast/progress.md`) – recent session reports/testing evidence.
- Compile-buffer evaluation (`doc/agents/ast/hb_compilebuf_evaluation.md`) – golden snapshot design.
- Verification matrix (this draft) – hbmk2 sweeps, cmocka suites, `scripts/test-ast.sh`, snapshot rules.
- If inheriting work, review the latest “Session transition” note before editing and preserve uncommitted changes.

### Milestone Ledger (2025-10-23)
| Milestone | Status | Notes |
| --- | --- | --- |
| Trace callback foundation | ✅ | `hbtraceast.c`, retain/release tests. |
| Lexer emission pass | ✅ | Tokens/boundaries published from `hb_comp_yylex`. |
| Parser hook pilot | ✅ | Functions/classes/control-flow instrumented; backlog tracks remaining reductions. |
| Stabilisation & toggles | ✅ | PP sink + CLI/env toggles documented. |
| Verification sweep | ✅ | `hbmk2-fixtures`, `compilebuf-tests`, `ast_trace_tests`, `scripts/test-ast.sh` integrated. |
| Golden traces & diagnostics | 🔄 | **Next**: single-module coverage, debug counters, golden JSON snapshots. |

### Active Objectives (next sessions)
1. **Single-module coverage** – instrument `hb_compParserRun` eager-token path; add regression tests.  
2. **Diagnostics toggles** – optional counters (token/node totals, retained traceinfo) gated by instrumentation flag.  
3. **Golden snapshots** – promote compile-buffer & CLI trace dumps to JSON fixtures in `tests/ast/fixtures/`; coordinate schema/tooling updates.  
4. **Docs & cleanup** – update `instrumentation-plan.md`, `hb_compilebuf_evaluation.md`, CLI/env docs.  
5. **Verification sweep** – rerun full matrix and capture command summaries for the session report.

> Aim for one objective per session. If stopping mid-step, leave code buildable, run available tests, and log the next action for the hand-off.

### Execution Checklist
**Start**  
1. Read latest entries in `doc/agents/ast/progress.md`.  
2. Confirm `git status`; review prior “Session transition” if work is in progress.  
3. Note which objective(s) you plan to tackle.

**During**  
- Keep feature flags disabled until stable.  
- Update docs/fixtures with related code changes.  
- Run targeted tests after each major edit.

**Before hand-off**  
1. Run applicable tests (`hbmk2 -w3`, cmocka suites, `scripts/test-ast.sh`, snapshot checks).  
2. Log results + completed work in `doc/agents/ast/progress.md`.  
3. Refresh this draft (milestones, objectives, follow-ups).  
4. Prepare commits or clearly state pending work if handing over without committing.

### Testing Expectations
- `tests/ast` cmocka suites: `ast_trace_tests`, `hbmk2-fixtures`, `compilebuf-tests`, `hbmk-ast-tests`.  
- `scripts/test-ast.sh`.  
- `hbmk2 -w3` on every affected `.prg`/`.ch` (no warnings allowed).  
- Golden trace comparison once fixtures are established.  
- Verify `HB_PP_TRACEINFO` retain/release balances return to zero.

### Reporting Template
Include in final session note (and summarise in commit message):
- Objectives completed (reference list above).  
- Tests executed (commands + pass/fail).  
- Docs/fixtures/code added or changed.  
- Remaining risks or TODOs.

### Session Transition Notes
- **Focus**: Extend CLI/compile-buffer trace validation—`hb_compMainExtModule()`, finish callback, and `--ast-trace`/`--ast-trace-dump=-` pipeline already landed; `tests/ast/hbmk-ast-tests` validates `fixture_demo.ast.json`.  
- **Pending**: Add additional fixtures, publish JSON snapshots via compile-buffer harness, document CLI/env usage (`HB_AST_TRACE`, `HB_AST_TRACE_DUMP`), clean temporary artefacts after tests.  
- **Prompt for next delegate**: “Record completed subtasks, outstanding items, test outcomes, and uncommitted file status in both `doc/agents/ast/progress.md` and `doc/agents/ast/draft.md` before ending the session. If work was inherited mid-task, describe exactly what remains.”

### Open Follow-ups
- Instrument `hb_compParserRun` (single-module coverage) and validate sequencing.  
- Add instrumentation debug counters/toggles.  
- Promote compile-buffer/CLI traces to golden JSON fixtures; define refresh workflow.  
- Expand `hbmk-ast-tests` fixture matrix; refresh CLI/env documentation.  
- Continue parser-hook backlog (expression reductions, error recovery, exit/return handling).
- **Current state**: `hb_compMainExtModule()` + `--ast-trace-dump` (`HB_AST_TRACE_DUMP`) landed; `hbtraceast.c` streams JSON via `hb_compAstTraceDumpJson`; `tests/ast/hbmk-ast-tests.c` runs `fixture_demo.prg` with `--ast-trace --ast-trace-dump=-`, scrubs the banner, and matches `tests/ast/fixtures/fixture_demo.ast.json`; `tests/ast/Makefile` and `.gitignore` already cover the harness.
- **Open items**:
  - [x] Fix CLI handling so `--ast-trace-dump=-` routes to stdout without triggering error `F0035` (2025-10-23: `cmdcheck.c` sentinel parsing tightened).
  - [x] Audit the dump path for formatting issues or leaks while capturing stdout (`hb_compAstTraceDumpJson` review + cmocka coverage).
  - [ ] Expand fixture coverage (additional `.prg`/`.ch` pairs) and update docs once traces stabilise.
  - [ ] Surface nightly build guidance for `HB_AST_TRACE_DUMP` / CLI usage.
- **Next session prep**: Compiler rebuilt and `tests/ast` suites (including `hbmk-ast-tests`) ran clean on 2025-10-23; fixture demo snapshot refreshed after restoring the trailing newline, so keep clearing temporary artefacts (`fixture_demo.c`, stdout dumps) after runs, then pivot to new fixtures + documentation refresh for the trace-dump workflow.

### Parser hook backlog
- [x] Class declarations / method definitions (trace node enter/leave, token mapping).
- [x] Statement-level constructs (IF/ELSE, DO WHILE, SWITCH, SEQUENCE, WITH, FOREACH).
- [x] Track codeblock literals (enter/leave around `{|| ... }`).
- [ ] Expression reductions beyond block literals (binary operators, macro expressions).
- [ ] Error recovery paths (ensure events flush correctly on syntax errors).
- [ ] Exit/return handling (ensure final boundary events align with node closures).
- [ ] Stand up integration fixtures that compile representative `.prg` snippets via `hb_compileBuf()` / `__pp_process()` and assert the emitted trace streams match golden snapshots (captures end-to-end spec coverage for tokens + AST).

## Delegation Brief – AST Tooling Agent
- **Mandate**: Rework the tooling layer (post-extraction) to consume compiler-emitted events, keeping schemas and fixtures aligned with Harbour instrumentation.
- **Scope**:
  - Replace the standalone lexer with consumers of `HB_AST_EVENT_TOKEN` and `HB_AST_EVENT_NODE_*` streams; update builders/serializers accordingly.
  - Update JSON/CBOR schema, documentation, and fixtures (`serialization-format.md`, `hbast-verify.md`, snapshots) to incorporate expansion IDs and token-node mappings.
  - Ensure extracted tooling repo mirrors Harbour hook expectations while staying decoupled from the core tree.
- **Task breakdown**:
  1. **Event ingestion harness** — build a thin adaptor that connects compiler event streams to the existing builder; document API assumptions and gaps.
  2. **Schema/serializer update** — retrofit serializers and `hbast.schema.json` for new fields; record revision notes and compatibility guidance.
  3. **Fixture refresh** — regenerate snapshots listed in the verification matrix, adding nested macro and dialect coverage; catalogue any failing scenarios for follow-up.
  4. **Documentation pass** — update `serialization-format.md`, `hbast-verify.md`, and related docs with new payload examples/validation rules.
  5. **Verification sweep** — run tooling cmocka and `scripts/test-ast.sh` after each milestone, noting results and pending work in the session report.
- **Incremental execution guidance**:
  - Close each milestone with a documentation stub in `doc/agents/ast/progress.md` and a to-do note in `doc/agents/ast/draft.md` so the next session can resume quickly.
  - Leave the tooling repo in a compilable/testable state between sessions; if a schema bump is mid-flight, gate it with a preview flag until fixtures catch up.
- **Dependencies & references**:
  - Alignment roadmap (tooling extraction week of 2025-10-27; integration week of 2025-11-03).
  - Hook map specifics on available payload fields.
  - Verification matrix for mandatory suites (`tests/ast` cmocka harness, `scripts/test-ast.sh`) and fixture upkeep.
- **Validation**:
  - Run tooling cmocka suites against compiler-delivered streams; include assertions about node/token coverage.
  - Execute `scripts/test-ast.sh` to compare generated artefacts to golden snapshots, updating them only when instrumentation changes are intentional.
  - Document schema/version changes and compatibility notes.
- **Deliverables**:
  - Updated tooling codebase (in separated repo) ready to ingest Harbour events.
  - Revised docs/fixtures and test logs demonstrating parity.
  - Report detailing unresolved coverage gaps or requested compiler hooks for future sessions.
  - Clearly marked follow-up items enabling a new session to pick up pending fixtures or docs without ambiguity.

## Oversight Session 2025-10-21 Notes
- Compiler instrumentation hooks (lifecycle, lexer, parser, expression helpers) verified against the plan; documentation updated with status tracking.
- cmocka `expression_nodes_capture_reductions` fixed to unblock trace-events target; `scripts/test-ast.sh` executed successfully.
- Outstanding work before hand-off: keep `hbmk2 -w3` in the regression rotation, assess single-module `hb_compParserRun` buffering, and continue expression backlog audit for remaining grammar reductions.
