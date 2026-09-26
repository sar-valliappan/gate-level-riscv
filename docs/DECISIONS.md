# DECISIONS.md

Chronological log of individual design decisions and why they were made.
For current-state architecture, see DESIGN.md.

---

Signal: How to storage signals

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