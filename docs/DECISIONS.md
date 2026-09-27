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

**Decision:** `Signal::value` is `Bit`, a three-valued enum (`Zero`, `One`, `X`), not `bool`. `Gate::eval` implements 4-valued NAND, where a `0` input forces the output high even against an `X`.

**Why:** Every gate output needs an initial value before it's ever evaluated, and a `bool` default is indistinguishable from a real settled value, whereas `X` honestly means "not yet computed."

**Tradeoff accepted:** Every consumer of a signal's value must handle three cases instead of two.

---

Simulation: stale in-flight events on reconvergent paths

**Decision:** `test_utils::settle()` tracks the value of its latest scheduled-but-not-yet-applied event per signal, and compares against that instead of the committed store value when deciding whether a gate's output changed.

**Why:** A signal with reconvergent fanout (e.g. XOR's shared NAND term) can be re-evaluated twice for one input change before either resulting event fires. Comparing only against the committed value let the first, since-superseded evaluation schedule an event that never got corrected, permanently locking in a wrong value.

**Tradeoff accepted:** `settle()` needs one extra hash map that a plain priority-queue drain didn't. Kept local to the test harness rather than changing `EventQueue`'s own ordering contract.

---

Simulation: bootstrapping bistable feedback loops

**Decision:** `test_utils::deposit()` schedules a signal's starting value as a time-0 event, like a Verilog `initial` block. `SignalStore::force()`/`release()` pin a signal to a concrete level regardless of what its driving gate computes.

**Why:** A cross-coupled NAND latch's `q`/`q_not` pair is two gates each reading the other's `X`, which evaluates to `X` forever — no sequence of ordinary events resolves that symmetry, the same way real hardware needs an explicit reset.

**Tradeoff accepted:** Every `Signal` carries a `forced` bool that `SignalStore::set()` must check.