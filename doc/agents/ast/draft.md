# Working scratchpad for notes and decisions

## 2025-10-18 Oversight Baseline Review
- Rewrote `Agents.md` to define oversight, implementation roles, and roadmap guardrails for the refactoring tooling mission.
- Logged current compiler vs tooling architecture alignment in `doc/agents/ast/progress.md`.
- Outstanding: draft instrumentation plan for compiler-integrated token/AST events and reconcile local `draft.md` edits prior to baseline commit.

## Follow-ups (Oversight Instrumentation Kickoff)
- DONE 2025-10-18: `doc/agents/ast/instrumentation-plan.md` documents hooks in `complex.c`, `harbour.y`, `hbcomp.c`, plus data contracts to tooling.
- DONE 2025-10-18: Delegation briefs prepared for Compiler Instrumentation Agent and AST Tooling Agent (see dedicated sections below).
- Define verification matrix for future commits (token parity fixtures, `PHB_EXPR` node coverage) before authorising implementation sessions.
- Track post-session follow-ups: expanded parser hooks and `hbmk2 -w3` run (event sink + instrumentation toggle landed 2025-10-20).

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
- [ ] Emit node enter/leave events for class declarations (`DECLARE CLASS`, `DECLARE MEMBER`).
- [ ] Capture statement-level reductions (IF/ELSE, FOR/NEXT, WHILE/ENDDO) with parent-child references.
- [ ] Track codeblock (`{|| ... }`) entry/exit, linking to originating tokens.
- [ ] Surface inline (`INLINE`) definitions and `INIT/EXIT` procedures with dedicated node kinds.
- [ ] Map macro-generated constructs to node events once traceinfo-to-node bindings are stabilised.

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
  3. Week of 2025-11-03: revisit instrumentation plan and begin drafting implementation briefs based on the stabilized core.

## Delegation Brief – Compiler Instrumentation Agent
- **Mandate**: Implement the compiler-side hooks in `doc/agents/ast/instrumentation-plan.md`, ensuring Harbour emits token and AST events backed by `HB_PP_TRACEINFO`.
- **Scope**:
  - Wire `hb_pp_setTraceCallback()` in `hb_comp_new()` / `hb_comp_free()` and manage `HB_PP_TRACEINFO` lifetimes.
  - Emit `HB_AST_EVENT_TOKEN` and boundary events from `hb_comp_yylex` (`src/compiler/complex.c`), aligning with the hook table.
  - Introduce guarded parser instrumentation in `src/compiler/harbour.y` (function declarations first), attaching stable node IDs and token references.
  - Respect feature toggles so tracing can be disabled without touching existing behaviour.
- **Task breakdown**:
  1. **Trace callback foundation** — implement registration/teardown (`hb_comp_new`, `hb_comp_free`), add stress tests for retain/release accounting. *Status: COMPLETED 2025-10-18 (trace helper in `hbtraceast.c`, cmocka `traceinfo_lifetime_balances`).*
  2. **Lexer emission pass** — instrument `hb_comp_yylex` with event emission and stable token IDs; capture sample event logs for fixtures in the verification matrix. *Status: COMPLETED 2025-10-18 (token/boundary events via `hb_compAstTracePublishToken/Return`).*
  3. **Parser hook pilot** — add reduction hooks for function declarations; produce a follow-up checklist for additional grammar nodes (stored below). *Status: COMPLETED 2025-10-18 (function enter/leave instrumentation in `harbour.y`).*
  4. **Stabilisation & notes** — update developer commentary/helpers as needed; highlight any helper APIs or toggles introduced. *In progress: event sink wiring and CLI toggle outstanding.*
  5. **Verification sweep** — run matrix suites after each milestone (hbmk2, cmocka, scripts/test-ast.sh), attaching summaries and outstanding issues to the session report. *Partial: cmocka + `scripts/test-ast.sh` pass; `hbmk2 -w3` pending.*
- **Incremental execution guidance**:
  - If the session approaches token limits, finish the current step, summarise partial results, and record next actions + test status in both `doc/agents/ast/progress.md` and a short note in `doc/agents/ast/draft.md`.
  - Each sub-step can be delivered as a separate delegated session; ensure code is left in a buildable/tested state with feature flags disabled by default if work is mid-flight.
- **Dependencies & references**:
  - Decision criteria (Phase 0) for what stays in core.
  - Alignment timeline (core hardening week of 2025-10-20).
  - Hook map and migration guidance in `doc/agents/ast/instrumentation-plan.md`.
  - Verification matrix for required test suites and fixtures.
- **Validation**:
  - `hbmk2 -w3` over affected fixtures; core regression suites as needed.
  - `tests/ast` cmocka harness (expanded with trace retention checks).
  - `scripts/test-ast.sh` driven by compiler events; verify snapshot parity.
  - Confirm `HB_PP_TRACEINFO` retain/release counts net to zero post-run.
- **Deliverables**:
  - Patched source files with instrumentation guards and any helper APIs.
  - Session report summarising hooks implemented, tests executed, and residual risks/open questions.
  - Updated log entries noting which parts of the task breakdown are complete and what remains for the next session.

### Parser hook backlog
- Class declarations / method definitions (trace node enter/leave, token mapping).
- Statement-level constructs (IF/ELSE, DO WHILE, SWITCH, SEQUENCE).
- Expression reductions (binary operators, macro expressions, lambda/block literals).
- Error recovery paths (ensure events flush correctly on syntax errors).
- Exit/return handling (ensure final boundary events align with node closures).

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
