# Performance Phase 1 Checklist

Goal: finish one complete architecture-first performance pass before the next user test.

- [x] Split the small performance panel from full debug rendering and heavy debug profiling
- [x] Split detailed entity profiling from the performance panel so the panel does not secretly enable extra runtime overhead
- [x] Introduce shared visual cache helpers so sprite/procedural cache policy is consistent and engine-owned
- [x] Add attachment topology caching so flat sprite attachment sets use dedicated fast update/draw paths instead of generic tree traversal

Notes:
- This phase avoids content compromises.
- This phase keeps stress visuals intact.
- This phase is intended to create a cleaner base for the next deeper render/physics changes.
