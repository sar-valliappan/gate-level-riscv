# DECISIONS.md

Chronological log of individual design decisions and why they were made.
For current-state architecture, see DESIGN.md.

---

Signal storage: index-based, not shared_ptr<Signal>

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