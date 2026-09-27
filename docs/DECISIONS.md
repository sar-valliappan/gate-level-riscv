# DECISIONS.md

Chronological log of individual design decisions and why they were made.
For current-state architecture, see DESIGN.md.

---

Signal: How to store signals

**Decision:** Store all signals in a flat `std::vector<Signal>` inside `SignalStore`, referenced everywhere by a lightweight `SignalId(uint32_t)` handle instead of `std::shared_ptr<Signal>`.

**Why:** Signals are read by many gates and written by the scheduler, requiring shared mutable access. 
`std::shared_ptr<Signal>` handles that via reference counting and heap-allocates each signal separately.
Index-based handles avoid both: no reference-counting overhead, and signal data stays contiguous in memory.

**Tradeoff accepted:** Callers need the store in scope to resolve a `SignalId` into a value.

---

Event queue: Event tiebreaker when events have the same timestamp

**Decision:** Order the event queue by `(time, seq)`, where `seq` is a monotonically increasing insertion counter. 
`seq` does not need to reset at every timestamp as it is monotonic, and the comparison logic will always hold.

**Why:** Same-timestamp events need a deterministic order for reproducibility/debugging. A `std::priority_queue` tie is otherwise unspecified.

--- 

Gate: How to store gates
 
**Decision:** Store all gates in a flat `std::vector<Gate>` inside `Circuit`, referenced everywhere by a lightweight `GateId(uint32_t)` handle, instead of by a copy of `Gate` itself.
 
**Why:** `Circuit::gates` is the single owning collection for gate data; every other structure that needs to refer to a gate holds its ID instead of a duplicate copy. Storing `std::vector<Gate>` in the fanout map instead would duplicate gate data across every fanout list a gate appears in and would require updating every duplicate if `Gate` ever gained a mutable field.
 
**Tradeoff accepted:** No meaningful tradeoff/improvement yet as `Gate` is immutable, but prevents future issues.

---

Signal: bool vs. tri-state values

**Decision:** `Signal::value` is `Bit`, a three-valued enum (`Zero`, `One`, `X`), not `bool`. `Gate::eval` implements 4-valued NAND: a `0` on either input forces the output to `1` regardless of the other input, even if that other input is `X`; otherwise any `X` input makes the result `X`.

**Why:** Every gate output needs *some* initial value before it can be evaluated, and a plain `bool` default is a lie — it looks like a settled `0` or `1` even though nothing has computed it yet. `X` gives that default an honest meaning ("not yet computed"), and it lets a circuit represent an uninitialized register or an undriven bus the way real HDLs (Verilog's `x`) do.

**Tradeoff accepted:** Every consumer of a signal's value (tests, and any future waveform output) must handle three cases instead of two. Feedback pairs in bistable elements (SR/D latch `q`/`q_not`) still need a concrete `Zero`/`One` seed rather than `X` — two cross-coupled NAND gates both reading `X` from each other evaluate to `X` forever and never converge, so `test_utils::initialize()` alone can't resolve them the way it does non-cyclic gate outputs.

---

Simulation: stale in-flight events on reconvergent paths

**Decision:** `test_utils::settle()` tracks, per signal, the value of its most recently scheduled-but-not-yet-applied event. A gate re-evaluation compares against that in-flight target (falling back to the committed `SignalStore` value if none is pending) rather than against the committed value alone, and a popped event whose value no longer matches the tracked target is discarded instead of applied.

**Why:** A signal with reconvergent fanout (e.g. XOR's shared NAND term, which feeds two gates that also read the changing input directly) can be re-evaluated twice for a single input change before either resulting event fires. Comparing only against the committed store value let the first, since-superseded evaluation schedule an event that later got applied verbatim, permanently locking in a wrong value with no further events left to correct it — this was caught by `gates_test`'s `xor_truth_table` case for `(1, 1)`, which settled to `1` instead of `0`.

**Tradeoff accepted:** `settle()` now does a small amount of extra bookkeeping (one hash map, keyed by signal) that a simpler priority-queue drain didn't need. This keeps the fix local to the test harness's simulation loop rather than changing `EventQueue`'s own ordering contract, which `event_test.cpp` relies on to deliver same-timestamp events for one signal in insertion order.