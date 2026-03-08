# Wevoa Coding Standards

1. Kernel code must be freestanding and avoid libc calls.
2. Keep module boundaries strict (`arch`, `dev`, `mm`, `fs`, `shell`, `core`, `lib`).
3. Use explicit-width integer types.
4. Avoid hidden global mutable state across modules unless unavoidable.
5. Keep assembly minimal and isolate architecture assumptions.
6. Every subsystem addition should include at least one smoke test scenario.

