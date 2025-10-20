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
- **Harbour core scope**: `include/hbpp.h`, `src/pp/ppcore.c`, `src/compiler/*`, `src/harbour.def`, and the instrumentation docs (`Agents.md`, `doc/agents/ast/progress.md`, this draft, `doc/agents/ast/hb_compilebuf_evaluation.md`) remain in-tree. They jointly define, exercise, and document the compiler trace pipeline.
- **Standalone tooling overlay**: the first-attempt artefacts (`src/ast/`, `utils/hbast`, `utils/hbrename`, legacy fixtures/tests/docs) were removed on 2025-10-25. Any future experimentation should live out of tree and consume the compiler trace APIs/CLI.
- **Focus going forward**: harden the compiler dump (`hb_compAstTraceDumpJson()`), expand test coverage via existing cmocka/CLI harnesses, and ensure documentation (`README-AST.md`, `serialization-format.md`) reflects the compiler-as-source-of-truth model.
- **Roadmap checkpoints**:
  1. Week of 2025-10-20: instrumentation hooks complete (done).
  2. Week of 2025-10-25: retire first-attempt tooling (done).
  3. Week of 2025-11-03: refresh docs/tests to cover extended dump metadata (macro IDs, diagnostics), stage trace-pack instructions, and plan the next fixture additions.

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
| Golden traces & diagnostics | 🔄 | Single-module coverage landed (`hb_compParserRun` instrumentation + `compilebuf-tests` `-m` harness); fixture matrix now spans `fixture_blocks`, `fixture_ppdirectives`, `fixture_statements`, `fixture_expressions`, and `fixture_includes` JSON dumps alongside dialect variants; diagnostics counters/toggles shipped; documentation + trace-pack packaging still pending to close the milestone. |

### Active Objectives (next sessions)
1. ✅ **Single-module coverage** – `hb_compParserRun` now routes through `hb_comp_yylex`; both compile-buffer and CLI harnesses iterate over `fixture_demo` and `fixture_blocks` under default/`-m` modes. Next: document the behaviour and backfill additional fixtures before snapshot promotion.  
2. ✅ **Diagnostics toggles** – CLI/env flag (`--ast-trace-diagnostics` / `HB_AST_TRACE_DIAGNOSTICS`) now exposes token/boundary/node/traceinfo totals for troubleshooting.  
3. 🔄 **Fixture snapshot freeze** – expression-heavy (`fixture_expressions`) and include-driven (`fixture_includes`) fixtures now ship with golden dumps + hbmk harness coverage; next step is to capture the regeneration commands (`hbmk2`, CLI dump) in the docs and stage the trace pack hand-off notes.  
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
- **2025-10-25 (fixture freeze prep)**: Added expression-heavy `fixture_expressions.prg` and include-driven `fixture_includes.prg`, plus the supporting `fixture_include_chain.ch`; regenerated CLI snapshots and registered both fixtures with `tests/ast/ast_hbmk_ast_tests.c`.  
  - **Commands**: `bin/linux/gcc/harbour -iinclude --ast-trace --ast-trace-dump=tests/ast/fixtures/fixture_expressions.ast.json tests/ast/fixture_expressions.prg`, `bin/linux/gcc/harbour -iinclude --ast-trace --ast-trace-dump=tests/ast/fixtures/fixture_includes.ast.json tests/ast/fixture_includes.prg`, `tests/ast/hbmk-ast-tests`, `bin/linux/gcc/hbmk2 -w3 tests/ast/fixture_expressions.prg`, `bin/linux/gcc/hbmk2 -w3 tests/ast/fixture_includes.prg`.  
  - **Packaging**: Core fixtures and snapshots bundled as `tests/ast/trace-pack/core-trace-pack-2025-10-25.zip` with regeneration notes in `tests/ast/trace-pack/README.md`.  
  - **Outstanding**: Expand compile-buffer snapshot coverage and keep the working tree limited to fixture/doc updates until those checks are logged (verification sweep completed 2025-10-25).
- **2025-10-24 (dialect fixtures)**: Added two dedicated fixtures, `fixture_compat_clipper.prg` and `fixture_compat_harbour.prg`, backed by `fixture_compat_common.ch`. Both now exercise multiple `BEGIN SEQUENCE`/`RECOVER` flows and emit their own golden snapshots. Updated `ast_hbmk_ast_tests.c` to include the pair and extended `ast_compilebuf_tests.c` with both Clipper and Harbour in-memory variants.  
  - **Working tree**: pending files in `.gitignore`, `tests/ast/ast_compilebuf_tests.c`, `tests/ast/ast_hbmk_ast_tests.c`, `tests/ast/fixture_compat_clipper.prg`, `tests/ast/fixture_compat_harbour.prg`, `tests/ast/fixture_compat_common.ch`, and the refreshed snapshots under `tests/ast/fixtures/`.
- **Focus** (next sessions):  
  1. ✅ Add the expression-heavy and include-driven fixtures, regenerate JSON, and document the commands (fixtures + snapshots landed 2025-10-25; documentation follow-up pending).  
  2. ✅ Freeze the “trace pack” (zip or tagged directory) containing all fixtures + snapshots; log its regeneration workflow (core bundle published at `tests/ast/trace-pack/core-trace-pack-2025-10-25.zip`).  
  3. ✅ Update docs (`instrumentation-plan.md`, `hb_compilebuf_evaluation.md`) with the final toggles/commands (snapshot workflow recorded 2025-10-25).  
  4. ✅ Run the verification matrix once more and record results (commands executed 2025-10-25; outcomes logged in `progress.md`).
- **Prompt for next delegate**: “Record completed subtasks, outstanding items, test outcomes, and uncommitted file status in both `doc/agents/ast/progress.md` and `doc/agents/ast/draft.md` before ending the session. If work was inherited mid-task, describe exactly what remains.”

### Open Follow-ups
- DONE 2025-10-25: Added expression-heavy (`fixture_expressions`) and include-driven (`fixture_includes`) samples with golden snapshots; regeneration workflow captured in `instrumentation-plan.md` and `hb_compilebuf_evaluation.md`.  
- [x] Add instrumentation debug counters/toggles (exposed via `--ast-trace-diagnostics` / `HB_AST_TRACE_DIAGNOSTICS`).  
- [x] Package the compile-buffer/CLI traces as the initial “trace pack”; document the regeneration workflow (see `tests/ast/trace-pack/core-trace-pack-2025-10-25.zip` and `README.md`).  
- Update `hbmk-ast-tests`/`ast_compilebuf_tests` documentation with the exact commands/flags used to refresh snapshots.  
- Continue the parser-hook backlog (expression reductions, error recovery, exit/return handling).
- **Current state**: `hb_compMainExtModule()` + `--ast-trace-dump` (`HB_AST_TRACE_DUMP`) landed; `hbtraceast.c` streams JSON via `hb_compAstTraceDumpJson`; `tests/ast/hbmk-ast-tests.c` runs `fixture_demo.prg` with `--ast-trace --ast-trace-dump=-`, scrubs the banner, and matches `tests/ast/fixtures/fixture_demo.ast.json`; `tests/ast/Makefile` and `.gitignore` already cover the harness.
- **Open items**:
- [x] Fix CLI handling so `--ast-trace-dump=-` routes to stdout without triggering error `F0035` (2025-10-23: `cmdcheck.c` sentinel parsing tightened).
- [x] Audit the dump path for formatting issues or leaks while capturing stdout (`hb_compAstTraceDumpJson` review + cmocka coverage).
- [x] Expand fixture coverage (additional `.prg`/`.ch` pairs) and update docs once traces stabilise — expression-heavy and include-driven fixtures landed 2025-10-25; doc refresh + trace-pack packaging still outstanding.
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

### Tooling migration note (2025-10-25)
The first-attempt tooling overlay (`src/ast/`, `utils/hbast`, `utils/hbrename`, legacy fixtures/tests/docs) was removed from Harbour. Any future experimentation should live in a separate repository and consume the compiler trace APIs or CLI dump directly. The milestone ledger, execution checklist, and testing expectations from the earlier plan are preserved in git history but no longer apply to the core tree.

## Delegation Brief – AST Tooling Migration Agent (Template)

Delegation Brief: you are the AST Tooling Migration Agent

### Overview
- **Mandate**: Audit the first-attempt AST tooling work (commit range `d29cad47f5f8025136caa89f5a92392d13d87751`‒`afa3c2c7012109d03c0ed6ee3ed94ea4d6b0426c`) that introduced parallel lexers, parsers, and utilities. Determine what should be merged into the compiler-backed flow, what belongs in a separate module, and what should be retired.  
- **Goal**: Deliver a migration/retirement plan that eliminates duplicated logic while preserving useful capabilities, keeping terminology aligned with the CA‑Clipper 5.3 guide (`doc/references/c53g01c.txt`) and Harbour docs.

### Inputs & References
- Git history: `git diff --stat d29cad47f5f8025136caa89f5a92392d13d87751..afa3c2c7012109d03c0ed6ee3ed94ea4d6b0426c`.  
- Files touched in that range (e.g., instrumentation hooks, `tests/ast` harness, documentation updates).  
- Current instrumentation plan (`doc/agents/ast/instrumentation-plan.md`) and progress log (`doc/agents/ast/progress.md`).  
- Latest compiler extensions (trace dump, diagnostics toggles) to understand replacement capabilities.

### Milestone Ledger
| Milestone | Status | Notes |
| --- | --- | --- |
| Inventory legacy tooling | ⏳ | Produce a file-by-file summary of features added in the first attempt. |
| Triage decision matrix | ⏳ | Classify each component: migrate into core extensions, keep as optional tooling, or retire. |
| Migration blueprint | ⏳ | Outline concrete tasks (code moves, test updates, docs) for items flagged “migrate”. |
| Retirement plan | ⏳ | Specify removal steps, cleanup patches, and verification for items flagged “retire”. |
| Module packaging notes | ⏳ | Define structure/ownership for tooling that remains standalone. |

### Step-by-Step Guide
1. **Gather evidence** – run `git log --stat` and targeted `git show` commands across the commit range to capture the scope of added files and functionality.  
2. **Classify components** – for each file/function introduced, decide whether it should migrate, remain as optional tooling, or be removed; note dependencies on current compiler hooks.  
3. **Draft migration tasks** – for “migrate” items, specify the target location (e.g., compiler trace workflow), tests to adapt, and docs that need updates.  
4. **Draft retirement tasks** – for “retire” items, list deletion steps, Makefile/documentation cleanup, and regression checks.  
5. **Recommend packaging** – define boundaries for tooling that stays separate (directory layout, build targets, doc updates).  
6. **Report findings** – log the classification and proposed actions in `doc/agents/ast/progress.md` and summarise the required follow-ups here.

> Treat this as an analysis-first role. Avoid large deletions until the overseer approves the plan; focus on clarity and actionable follow-ups.

### Execution Checklist
**Start**  
- Confirm `git status` is clean (analysis-only session).  
- Prepare diff summaries for the commit range.  

**During**  
- Use a scratchpad (`hb-ast-temp.md`) for notes before transcribing results.  
- Cross-reference existing compiler capabilities (post `1ff54bcd6d07`) to avoid recommending duplicate work.  
- Capture command snippets for reproducibility.

**Before hand-off**  
- Add a concise report (inventory, classifications, proposed actions) to `doc/agents/ast/progress.md`.  
- Update this draft with TODO checkboxes for implementation agents.  
- Leave the codebase untouched unless a blocker fix is unavoidable; document any blockers clearly.

### Testing Expectations
- None for pure analysis.  
- If prototype cleanups occur, run `make -C tests/ast tests`, `scripts/test-ast.sh`, and `hbmk2 -w3` on affected fixtures to confirm parity.

### Reporting Template
- Summary of files reviewed / commands executed.  
- Decision matrix (migrate / keep / retire) with reasoning.  
- Proposed follow-up tasks grouped by agent or component.  
- Risks or open questions for overseer review.

### Session Transition Note
- If analysis is incomplete, list remaining directories or commits.  
- Flag any missing compiler hooks that would block migration.  
- Record agreed next steps for implementation agents (e.g., “Compiler Instrumentation Agent to expose X”, “AST Tooling Agent to remove Y”).

### Open Follow-ups
- [x] Complete the inventory and classification of the first-attempt tooling commits.  
- [ ] Extend `hb_compAstTraceDumpJson()` so consumers get deterministic macro bookkeeping (stable IDs, depth, call ranges) straight from the compiler with no external adapter.  
- [x] Update tooling docs/tests to reference the compiler CLI (`harbour --ast-trace --ast-trace-dump`) and retire the legacy `hbast`/`hbrename` workflows.  
- [ ] Define the packaging plan for tooling that stays standalone (directory layout, build targets, ownership).  
- [ ] Prepare phased cleanup patches to remove `src/ast/lexer`, legacy cmocka suites, and `utils/hbrename` once the replacement path is ready.  
- [ ] Update documentation (`Agents.md`, `serialization-format.md`, `README-AST.md`) to reflect the compiler-backed flow and note retired modules.  
- [ ] Coordinate with the Compiler Instrumentation Agent on any missing hooks or telemetry needed by the consumer adapter before deletions proceed.  

## Oversight Session 2025-10-21 Notes
- Compiler instrumentation hooks (lifecycle, lexer, parser, expression helpers) verified against the plan; documentation updated with status tracking.
- cmocka `expression_nodes_capture_reductions` fixed to unblock trace-events target; `scripts/test-ast.sh` executed successfully.
- Outstanding work before hand-off: keep `hbmk2 -w3` in the regression rotation, assess single-module `hb_compParserRun` buffering, and continue expression backlog audit for remaining grammar reductions.
