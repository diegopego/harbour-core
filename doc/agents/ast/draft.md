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
  - `hbmk2 -w3` must succeed with **zero warnings** for every `.prg`, `.ch`, `.hbm`, `.hbmk`, or related fixture touched by instrumentation or tooling work before merge.
  - Harbour compiler self-tests covering macro-heavy code paths where traces are emitted.
- **Tooling harness**:
  - `tests/ast` cmocka suites: extend to validate that emitted token events and AST node events match expected shapes; include leak detectors for `HB_PP_TRACEINFO`.
  - `scripts/test-ast.sh`: rerun snapshot comparisons using compiler-sourced events; failures indicate divergence from the schema/fixtures.
- **Run log**:
  - 2025-10-22: `hbmk2 -w3` sweeps logged; `hbmk2-fixtures` cmocka target now enforces the warning-free requirement, `hb_compMainExt()` finish callback and `hb_compMainExtModule()` enable instrumentation harvesting, CLI trace-dump (`--ast-trace`, `--ast-trace-dump`, `HB_AST_TRACE_DUMP`) produces the JSON streams validated by `tests/ast/hbmk-ast-tests`, and `doc/agents/ast/hb_compilebuf_evaluation.md` records the `hb_compileBuf` golden-plan.
- **Fixtures & snapshots**:
  - Golden snapshots must originate from compiler instrumentation (CLI trace dump, compile-buffer harness); reuse and expand `tests/ast/fixtures/*.ast.json` as the authoritative source.
  - Add variants for nested macros, dialect switches, and compile-buffer cases as coverage grows; keep snapshots in tooling repo but reference them from Harbour core for parity checks.
- **Pass/Fail criteria**:
  - All suites above must pass; `hbmk2 -w3` must report zero warnings/errors across the fixture set.
  - Token IDs emitted by `hb_comp_yylex` should reconcile with nodes collected in parser reductions (no orphan tokens/nodes).
  - `HB_PP_TRACEINFO` retain/release counts must net to zero after each test run (cmocka assertion).
  - Any intentional change to golden snapshots must be regenerated via compiler instrumentation and documented.
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
Delegation Brief: you are the Compiler Instrumentation Agent

### Overview
- **Mandate**: Keep Harbour’s compiler as the single source of truth by emitting token, boundary, node, and preprocessor events backed by `HB_PP_TRACEINFO` (see `doc/agents/ast/instrumentation-plan.md`). All `.prg`/`.ch`/`.hbm`/`.hbmk` fixtures must remain warning-free under `hbmk2 -w3`; compiler-produced traces (CLI dump, compile-buffer) are the golden reference.
- **Scope**: Manage PP trace lifecycle (`hb_pp_setTraceCallback`), instrument `hb_comp_yylex`, parser reductions (`harbour.y` / `hbmain.c`), feature toggles, and trace dump/compile-buffer paths.

### Inputs & References
- Phase 0 decision criteria & parser-hook backlog (this draft).
- Instrumentation plan (`doc/agents/ast/instrumentation-plan.md`) – hook map, run log, open questions (golden snapshots, diagnostics counters).
- Progress log (`doc/agents/ast/progress.md`) – recent session reports/testing evidence.
- Compile-buffer evaluation (`doc/agents/ast/hb_compilebuf_evaluation.md`) – golden snapshot design.
- Verification matrix (this draft) – hbmk2 sweeps, cmocka suites, `scripts/test-ast.sh`, snapshot rules.
- If inheriting work, review the latest “Session transition” note before editing and preserve uncommitted changes.

### Milestone Ledger (2025-10-24)
| Milestone | Status | Notes |
| --- | --- | --- |
| Trace callback foundation | ✅ | `hbtraceast.c`, retain/release tests. |
| Lexer emission pass | ✅ | Tokens/boundaries published from `hb_comp_yylex`. |
| Parser hook pilot | ✅ | Functions/classes/control-flow instrumented; backlog tracks remaining reductions. |
| Stabilisation & toggles | ✅ | PP sink + CLI/env toggles documented. |
| Verification sweep | ✅ | `hbmk2-fixtures`, `compilebuf-tests`, `ast_trace_tests`, `scripts/test-ast.sh` integrated. |
| Golden traces & diagnostics | 🔄 | Single-module coverage landed (`hb_compParserRun` instrumentation + `compilebuf-tests` `-m` harness); fixture matrix now includes `fixture_blocks`, `fixture_ppdirectives`, and `fixture_statements` JSON dumps; diagnostics counters/toggles shipped; pending broader golden snapshot workflow. |

### Active Objectives (next sessions)
1. ✅ **Single-module coverage** – `hb_compParserRun` now routes through `hb_comp_yylex`; both compile-buffer and CLI harnesses iterate over `fixture_demo` and `fixture_blocks` under default/`-m` modes. Next: document the behaviour and backfill additional fixtures before snapshot promotion.  
2. ✅ **Diagnostics toggles** – CLI/env flag (`--ast-trace-diagnostics` / `HB_AST_TRACE_DIAGNOSTICS`) now exposes token/boundary/node/traceinfo totals for troubleshooting.  
3. **Fixture snapshot freeze** – lock the minimal fixture matrix (existing ones plus expression-heavy and include-driven samples), capture the exact regeneration commands (`hbmk2`, `hb_compMainExtModule`), and document the workflow prior to tooling hand-off.  
4. **Trace pack publication** – bundle the frozen fixtures/JSON as an initial “trace pack” and record distribution/testing instructions so tooling can consume it directly.  
5. **Docs & cleanup** – update `instrumentation-plan.md`, `hb_compilebuf_evaluation.md`, CLI/env docs with the new toggles and fixture workflow.  
6. **Verification sweep** – rerun the full matrix once the trace pack is frozen, logging commands and results.

> Aim for one objective per session. If stopping mid-step, leave code buildable, run available tests, and log the next action for the hand-off.

### Execution Checklist
**Start**  
1. Read latest entries in `doc/agents/ast/progress.md`.  
2. Confirm `git status`; review prior “Session transition” if work is in progress.  
3. Note which objective(s) you plan to tackle.  
4. Tooling on hand (use only when it meaningfully shortens the task): `jq` / `jsonlint` for trace JSON inspection, `tree-sitter` CLI for quick grammar tests. When naming new fixtures, tokens, or events, consult the CA‑Clipper 5.3 guide (`doc/references/c53g01c.txt`) and Harbour docs first so terminology stays aligned with the language’s canon.

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
- `hbmk2 -w3` on every affected `.prg`/`.ch`/`.hbm`/`.hbmk` (no warnings allowed).  
- Golden trace comparison against compiler-generated JSON snapshots (CLI dump, compile-buffer harness).  
- Verify `HB_PP_TRACEINFO` retain/release balances return to zero.
- When invoking the compiler from tests or manual repros, include `-iinclude` so standard Harbour headers (e.g. `error.ch`) resolve the same way the harness does.

### Dialect Fixture Workflow
- Sources: `tests/ast/fixture_compat_clipper.prg` (Clipper pragmas: `-kh- -km+ -ko-`) and `tests/ast/fixture_compat_harbour.prg` (Harbour pragmas: `-kh+ -km- -ko+`) share helpers in `tests/ast/fixture_compat_common.ch`. Golden outputs live in `tests/ast/fixtures/fixture_compat_clipper.ast.json` and `tests/ast/fixtures/fixture_compat_harbour.ast.json`.
- CLI reproduction: `bin/linux/gcc/harbour -iinclude --ast-trace --ast-trace-dump=tests/ast/fixtures/<name>.ast.json tests/ast/<fixture>.prg`.
- Harness coverage: `tests/ast/hbmk-ast-tests` runs both fixtures in default and `-m` modes; `tests/ast/compilebuf-tests` covers the Clipper variant today and can add the Harbour snapshot once the workflow is locked.
- FINALLY: instead of try/catch/finally harbour uses begin sequence/recover/always for error handling.

### Reporting Template
Include in final session note (and summarise in commit message):
- Objectives completed (reference list above).  
- Tests executed (commands + pass/fail).  
- Docs/fixtures/code added or changed.  
- Remaining risks or TODOs.

### Session Transition Notes
- **2025-10-24 (dialect fixtures)**: Added two dedicated fixtures, `fixture_compat_clipper.prg` and `fixture_compat_harbour.prg`, backed by `fixture_compat_common.ch`. Both now exercise multiple `BEGIN SEQUENCE`/`RECOVER` flows and emit their own golden snapshots. Updated `ast_hbmk_ast_tests.c` to include the pair and extended `ast_compilebuf_tests.c` with both Clipper and Harbour in-memory variants.  
  - **Working tree**: pending files in `.gitignore`, `tests/ast/ast_compilebuf_tests.c`, `tests/ast/ast_hbmk_ast_tests.c`, `tests/ast/fixture_compat_clipper.prg`, `tests/ast/fixture_compat_harbour.prg`, `tests/ast/fixture_compat_common.ch`, and the refreshed snapshots under `tests/ast/fixtures/`.
- **Focus** (next sessions):  
  1. Add the expression-heavy and include-driven fixtures, regenerate JSON, and document the commands.  
  2. Freeze the “trace pack” (zip or tagged directory) containing all fixtures + snapshots; log its regeneration workflow.  
  3. Update docs (`instrumentation-plan.md`, `hb_compilebuf_evaluation.md`) with the final toggles/commands.  
  4. Run the verification matrix once more and record results.
- **Prompt for next delegate**: “Record completed subtasks, outstanding items, test outcomes, and uncommitted file status in both `doc/agents/ast/progress.md` and `doc/agents/ast/draft.md` before ending the session. If work was inherited mid-task, describe exactly what remains.”

### Open Follow-ups
- Finish the fixture matrix (expression-heavy and include-driven samples) before freezing the trace pack.  
- [x] Add instrumentation debug counters/toggles (exposed via `--ast-trace-diagnostics` / `HB_AST_TRACE_DIAGNOSTICS`).  
- Package the compile-buffer/CLI traces as the initial “trace pack”; document the regeneration workflow.  
- Update `hbmk-ast-tests`/`ast_compilebuf_tests` documentation with the exact commands/flags used to refresh snapshots.  
- Continue the parser-hook backlog (expression reductions, error recovery, exit/return handling).
- **Current state**: `hb_compMainExtModule()` + `--ast-trace-dump` (`HB_AST_TRACE_DUMP`) landed; `hbtraceast.c` streams JSON via `hb_compAstTraceDumpJson`; `tests/ast/hbmk-ast-tests.c` runs `fixture_demo.prg` with `--ast-trace --ast-trace-dump=-`, scrubs the banner, and matches `tests/ast/fixtures/fixture_demo.ast.json`; `tests/ast/Makefile` and `.gitignore` already cover the harness.
- **Open items**:
- [x] Fix CLI handling so `--ast-trace-dump=-` routes to stdout without triggering error `F0035` (2025-10-23: `cmdcheck.c` sentinel parsing tightened).
- [x] Audit the dump path for formatting issues or leaks while capturing stdout (`hb_compAstTraceDumpJson` review + cmocka coverage).
- [ ] Expand fixture coverage (additional `.prg`/`.ch` pairs) and update docs once traces stabilise — Clipper/Harbour fixtures now cover nested recovery paths before locking snapshots.
- [ ] Surface nightly build guidance for `HB_AST_TRACE_DUMP` / CLI usage.
- [x] Document the new `-iinclude` requirement introduced for `hbmk-ast-tests` and compile-buffer harnesses.
- **Next session prep**: Compiler rebuilt and `tests/ast` suites (including `hbmk-ast-tests`) ran clean on 2025-10-23; fixture demo snapshot refreshed after restoring the trailing newline, so keep clearing temporary artefacts (`fixture_demo.c`, stdout dumps) after runs, then pivot to new fixtures + documentation refresh for the trace-dump workflow.

### Diagnostics toggle milestone (2025-10-24)
- Added diagnostics controls (`--ast-trace-diagnostics`, `HB_AST_TRACE_DIAGNOSTICS`) so runs can surface counter-only telemetry; `hb_compChkParseSwitch()` and `HB_COMP` now track the flag throughout the compilation.
- `hbtraceast.c` records lifecycle-safe counters for tokens, boundaries, nodes, preprocessor events, and traceinfo retain/release pairs while still emitting full JSON when tracing is enabled.
- `tests/ast/ast_trace_tests.c` picked up coverage for the diagnostics toggle and counter validation; docs (`progress.md`, `instrumentation-plan.md`, this draft) capture the milestone.
- Extended CLI fixture coverage with `tests/ast/fixture_blocks.prg` / `.ast.json`, keeping both default and `-m` modes green under the new diagnostics path.

### Parser hook backlog
- [x] Class declarations / method definitions (trace node enter/leave, token mapping).
- [x] Statement-level constructs (IF/ELSE, DO WHILE, SWITCH, SEQUENCE, WITH, FOREACH).
- [x] Track codeblock literals (enter/leave around `{|| ... }`).
- [ ] Expression reductions beyond block literals (binary operators, macro expressions).
- [ ] Error recovery paths (ensure events flush correctly on syntax errors).
- [ ] Exit/return handling (ensure final boundary events align with node closures).
- [ ] Stand up integration fixtures that compile representative `.prg` snippets via `hb_compileBuf()` / `__pp_process()` and assert the emitted trace streams match golden snapshots (captures end-to-end spec coverage for tokens + AST).

## Delegation Brief – AST Tooling Agent (Template)

Delegation Brief: you are the AST Tooling Agent

### Overview
- **Mandate**: Rework the tooling distribution so it consumes Harbour’s compiler-emitted events (token, boundary, node, macro traces) and generates snapshots aligning with the schema documented in `doc/agents/ast`. Tooling fixtures must compile warning-free via `hbmk2 -w3`, and compiler-derived traces constitute the golden snapshots.
- **Scope**: Replace standalone lexer/builder with compiler-fed pipeline, update serializers/schemas, refresh fixtures, and keep tooling decoupled while honouring Harbour’s truth.

### Inputs & References
- `doc/agents/ast/instrumentation-plan.md` – event payload definitions and hook status.
- `doc/agents/ast/progress.md` – latest instrumentation outputs and testing notes.
- Verification matrix (this draft) – suites required before acceptance.
- `doc/agents/ast/hb_compilebuf_evaluation.md` – golden snapshot plan interfacing with tooling.
- Current tooling repo status (branch: ast-3rd-experiment extraction tasks).

### Milestone Ledger (MVP Track)
| Milestone | Status | Notes |
| --- | --- | --- |
| Trace pack integration | ⏳ | Consume the compiler-provided fixtures/JSON (trace pack) instead of the old lexer output. |
| Event ingestion harness | ⏳ | Subscribe to `HB_AST_EVENT_*` streams (tokens, nodes, boundaries, PP) and feed the tooling builder. |
| Rename MVP | ⏳ | Implement a minimal refactoring (symbol rename) powered by compiler events. |
| Serializer/schema sync | ⏳ | Align tooling schema (`hbast.schema.json`, serializers) with the new event payloads. |
| Fixture verification | ⏳ | Add tests that replay the trace pack and confirm the tooling updates symbols correctly. |
| Documentation refresh | ⏳ | Capture the new ingestion workflow, CLI commands, and trace pack usage. |

### Step-by-Step Guide (MVP Path)
1. **Import the trace pack** – wire the tooling repo to load the compiler-generated JSON/TOML bundle; document expected layout (`fixture_*.prg`, `.ast.json`).  
2. **Event ingestion** – build a thin adaptor that turns the JSON traces (tokens / boundaries / nodes / macros) into the internal AST/symbol structures.  
3. **Rename prototype** – implement a rename symbol command using the compiler-derived AST, with fixtures proving Clipper/Harbour pragmas behave.  
4. **Serializer/schema alignment** – update schemas/serializers and record migrations; ensure the rename output respects CA‑Clipper/Harbour terminology.  
5. **Test harness** – extend existing cmocka/LSP tests to replay the trace pack and assert rename output matches expectations.  
6. **Docs & hand-off** – update `serialization-format.md`, `hbast-verify.md`, and repo README with the new workflow and commands (`hbmk2 -w3`, `hb_compMainExtModule`, etc.).

> Deliver one milestone per session when possible. If pausing mid-step, leave the tooling repo buildable, run partial tests, and log what remains.

### Execution Checklist
**Before starting**
- Review instrumentation updates so ingestion matches emitted payloads.
- Confirm the tooling repo branch state (clean or intended worktree changes).

**During**
- Integrate compiler APIs incrementally; keep fallbacks (prototype lexer) until migration is complete.
- Maintain toggle/config to switch between compiler-fed and prototype modes until parity verified.
- Update fixtures/docs in lockstep with code.

**Before hand-off**
- Run tooling cmocka suites (`make -C tests/ast tests` as applicable) and `scripts/test-ast.sh`.
- Regenerate golden snapshots and note any schema revisions (including version bumps).
- Document completed work and TODOs in `doc/agents/ast/progress.md`.
- Update this draft with milestone status and new follow-ups.

### Testing Expectations
- Tooling cmocka target (`tests/ast` harness in extracted repo) covering builder, serializer, verify CLI.
- `scripts/test-ast.sh` (or equivalent) to exercise end-to-end snapshot generation.
- CLI/LSP harnesses if applicable (e.g., `hbast verify`, `hbast dump`).
- `hbmk2 -w3` over every `.prg`/`.ch`/`.hbm`/`.hbmk` fixture involved; zero warnings permitted.
- Golden snapshot comparison checks – hash/JSON diff vs compiler-generated fixtures (CLI trace dump, compile-buffer outputs).

### Reporting Template
Each session should produce a summary covering:
- Milestones completed (refer to Step-by-Step list).
- Tests executed (commands + pass/fail).
- Fixtures/docs touched.
- Remaining issues or dependencies on compiler instrumentation.

### Session Transition Note
- If migration from the legacy lexer is mid-flight, ensure both paths are buildable and gated by runtime flags.  
- Record partial trace-pack consumption (list fixture names) and note whether outputs were provisional or committed.  
- Highlight any compiler instrumentation gaps blocking rename/LSP work.

### Open Follow-ups
- Import the compiler trace pack and retire the standalone lexer path once tests pass.  
- Build the rename MVP using compiler events, then expand to additional refactorings.  
- Sync schema/serializers with the new payloads and define a versioning policy.  
- Regenerate snapshots from the trace pack when schemas change; keep documentation (`hbast-verify.md`, README) aligned.  
- Plan LSP/tooling integration (go-to-definition, rename) once compiler-driven snapshots are stable.

## Oversight Session 2025-10-21 Notes
- Compiler instrumentation hooks (lifecycle, lexer, parser, expression helpers) verified against the plan; documentation updated with status tracking.
- cmocka `expression_nodes_capture_reductions` fixed to unblock trace-events target; `scripts/test-ast.sh` executed successfully.
- Outstanding work before hand-off: keep `hbmk2 -w3` in the regression rotation, assess single-module `hb_compParserRun` buffering, and continue expression backlog audit for remaining grammar reductions.
