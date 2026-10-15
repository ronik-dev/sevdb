# Changelog
All notable changes to this project will be documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.4.0] - 2026-10-15
### Changed
- Breaking Change: Replaced boolean and pointer return types with typed state enums (`sevdb_state`, `hm_state`, `pq_state`) **across the entire public API** to provide granular error reporting (e.g., out of memory, full capacity, duplicate IDs, missing IDs).
- Breaking Change: Refactored object creation and data retrieval functions to use output parameters (e.g., sevdb_vector **result, int *out_count) alongside state enum return values.
- Updated Doxygen documentation across `sevdb.h`, `hashmap.h`, and `pqueue.h` to accurately reflect the new double-pointer API contracts and ownership rules.
- Updated `ARCHITECTURE.md` to document the new double-pointer and state-enum memory model.
- Refactored all Criterion test suites and the standalone benchmark.c harness to comply with the new API signatures and state assertions.

### Fixed
- Fixed bug in `sevdb_db_remove_vector_by_id`: handled previoulsy ignored return value of `hm_update_value`, which could have resulted in inconsistent state.
- Fixed a bug in `hm_insert_new` where the bucket insertion index was not recalculated after an automatic capacity growth, causing elements to be inserted into mathematically incorrect buckets.
- Fixed a bug in `sevdb_db_search_k_similar_vectors` introduced by the refactor for the new changes in pqueue, that made the search fail when the priority queue replacement actually succeeded.

## [0.3.0] - 2026-10-08
### Added
- Implemented a custom, linear-probing integer hash map (`hashmap.c`/`hashmap.h`) with automatic load-factor resizing and tombstone deletion.
- Added a `hashmap_test_suite` to validate collision resolution, resizing, and tombstone logic.

### Changed
- Architectural Change: sevdb_database now utilizes a Sparse Set pattern (a dense array paired with a hash map).
- `sevdb_db_push_vector` now operates in O(1) average time, utilizing the hash map for duplicate ID rejection.
- `sevdb_db_get_vector_by_id` now operates in O(1) average time via direct hash map lookups.
- `sevdb_db_remove_vector_by_id` now operates in O(1) average time utilizing a swap-and-pop technique, eliminating gaps in the dense array and preserving cache locality for search.


## [0.2.0] - 2026-10-07
### Changed
- `sevdb_db_push_vector` now rejects a vector whose id already exists in the database, returning `NULL`.  
    On rejection, ownership of the vector is never transferred the caller is responsible for destroying it.
- **Breaking change**: `sevdb_db_search_k_similar_vectors` now outputs similarity scores alongside vectors. 
    The output array parameter now requires the `sevdb_similarity_scored_vector` struct defined in `types.h`.

### Fixed
- Fixed a signed/unsigned comparison in `sevdb_db_push_vector`'s free-slot scan (`int i` compared against `uint32_t capacity`).
- Fixed configuration file issue in CMakeLists that prevented correct import an use in other projects.
- Getter function signatures now enforce const correctness for their pointer arguments.

## [0.1.0] - 2026-09-27
### Added
- Vector and database lifecycle management (create/destroy) with a callee-allocates heap model.
- CRUD operations: push, get-by-id, remove-by-id, capacity growth.
- Euclidean distance and cosine similarity metrics.
- Top-K similarity search backed by a binary heap priority queue.
- Binary serialization/deserialization with CRC32 checksums and atomic temp-file writes.
- Criterion-based test suites (internal, distance, priority queue) and a benchmark harness.
- CI pipeline building and testing the project on every push and pull request to `main`.
- Versioned CMake build (`project(sevdb VERSION ...)`) with a generated `version.h`.
- Project documentation: `ARCHITECTURE.md` and `CONTRIBUTING.md`.
