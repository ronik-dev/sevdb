# sevdb Architecture

This document explains the *why* behind sevdb's design decisions, the tradeoffs considered and the reasoning that led to the current structure. 
It complements `docs/devlogs/`, which tracks the day-to-day *what* and *when*; this file is the curated, standing explanation for anyone reading the codebase without that history.

## Goals and non-goals

sevdb is a small embedded vector database written in C17, built primarily as a learning exercise in C and as a lightweight retrieval backend for a personal RAG project. 
It deliberately does **not** aim to be a production-grade vector store.
Concretely, it does not (at least for now) provide:

- Authentication or authorization
- Encryption of stored or serialized data
- A query language: all functionality is exposed via the C API directly
- Concurrent/multi-threaded access

## Public API surface and encapsulation

The library exposes two opaque types, `sevdb_vector` and `sevdb_database`, declared in `include/sevdb/types.h` as incomplete forward declarations:

```c
typedef struct sevdb_vector sevdb_vector;
typedef struct sevdb_database sevdb_database;
```

Their actual field layout lives in `src/sevdb_private.h`, which is never installed or exposed to consumers. 
This is the standard C opaque-pointer pattern, chosen for two reasons:

1. **ABI stability.** Consumers only ever hold a pointer; internal fields can be reordered, added, or removed without breaking code compiled against an older header, as long as the function signatures stay stable.
2. **Enforced encapsulation.** Because callers cannot see `struct sevdb_vector`'s fields, they are forced through the accessor functions (`sevdb_vector_get_id`, etc.) rather than reaching in directly.

All construction and destruction happens through paired functions (`sevdb_vector_create`/`sevdb_vector_destroy`, `sevdb_db_create`/`sevdb_db_destroy`), so the library, not the caller, decides the allocation strategy.

## Memory model: "callee allocates," heap-based, flexible array members

sevdb uses a **callee-allocates** convention throughout: functions that produce an object return a heap pointer the caller owns and must eventually destroy via the matching `_destroy` function. Nothing is returned by value or expected to live on the caller's stack.

`sevdb_vector` is defined with a C99 flexible array member:

```c
struct sevdb_vector {
    uint32_t id;
    uint32_t dimensions;
    float components[];
};
```

This means a vector's header and its component data are a **single contiguous allocation** (`sizeof(sevdb_vector) + dimensions * sizeof(float)`), sized exactly to the vector's dimensionality at creation time.
The alternative (storing a separate `float*` pointer to a second allocation) was rejected because it would double the allocations (and frees) per vector and scatter the components away from their header in memory, hurting cache locality during distance calculations that are already the hot path.

### Ownership transfer on push

`sevdb_db_push_vector` transfers ownership of the vector to the database on success: from that point, the database is responsible for freeing it (which happens in `sevdb_db_destroy` or `sevdb_db_remove_vector_by_id`), and the caller must not call `sevdb_vector_destroy` on it themselves. 
The function will reject a vector if the database already contains another with the same id. In this case ownership is not transfered, and the caller should manually destroy the vector.
This is documented in `sevdb.h`'s Doxygen comments and is the one place in the API where ownership crosses a boundary, worth knowing before extending the API, since introducing a second ownership-transfer point without equally clear documentation is an easy way to create a double-free.

## Data structure: dense pointer array, not a hash map

`sevdb_database` stores vectors in a flat array of pointers:

```c
struct sevdb_database {
    uint32_t capacity;
    uint32_t count;
    sevdb_vector **vectors;
};
```

Lookup by ID (`sevdb_db_get_vector_by_id`), similarity search, and iteration over all vectors during serialization are all linear scans over this array. 
This was a deliberate starting point, not a final answer: it's the simplest structure that satisfies the current test suite and lets everything else (search, persistence, capacity growth) be built and understood first. 
It also means similarity search, which must inspect every vector's components regardless of the storage structure, isn't actually made asymptotically worse by the array, the cost is dominated by the distance calculation itself, not the lookup.

The known cost is O(n) ID lookup and O(n) removal, and free slots are found by scanning for the first `NULL` on push. 
This is the primary target noted in `docs/devlogs/` as **"consider better data structure for storing vectors"**, a hash map keyed by ID would fix point lookups, but the tradeoff (extra memory, more complex serialization, needing search to still touch every vector for now) hasn't yet been worth taking on for the current scale.

### Capacity growth

`sevdb_db_increase_capacity` grows the backing array via `realloc` and zeroes the newly added region so it's indistinguishable from a freshly created database of the larger capacity. 
Growth is one-directional (capacity only ever increases) there is no shrink-to-fit operation.

## Similarity search: top-K via a bounded priority queue

`sevdb_db_search_k_similar_vectors` performs a **linear scan** over the database, scoring each candidate by cosine similarity against the query vector (`get_cosine_similarity` in `distance.c`), and maintains the current best-K candidates in a **fixed-capacity binary heap** (`pqueue.c`).

The key design choice here is that `pqueue` is **generic over ordering** via a comparator function pointer (`pq_compare_fn`), rather than being hardcoded to either a min-heap or max-heap:

```c
typedef bool(*pq_compare_fn)(float a, float b);
```

This lets the same priority queue implementation serve as a *min-heap* bounded to the K-best when the metric is "smaller is better" (Euclidean distance) or a *max-heap* when "larger is better" (cosine similarity): the caller supplies `is_min_heap`/`is_max_heap` at creation time rather than the queue baking in an assumption about metric direction. 
`sevdb_db_search_k_similar_vectors` bounds the heap to capacity `k` and only replaces the current worst element when a new candidate beats it, which keeps the search's extra memory at O(k) regardless of database size, at the cost of O(n log k) time for the full scan, appropriate for a linear-scan baseline, and the natural place to plug in an approximate algorithm (see Future Directions) without changing the heap itself.

NaN priorities are explicitly rejected at `pq_enqueue`, since a NaN would silently break heap ordering (NaN comparisons are always false, which can corrupt the heap invariant in ways that are hard to detect after the fact).

## Persistence: versioned binary format with checksummed atomic writes

The on-disk format (documented in full in `sevdb.h` and `persistence.c`) is a flat binary layout:

```
Header:  magic "SEVDB" (5 bytes) | version (u32) | capacity (u32) | count (u32)
Vector*: id (u32) | dimensions (u32) | components (f32 * dimensions)
Footer:  crc32 checksum (u32)
```

Three decisions here are worth calling out explicitly:

- **Explicit version field.** `sevdb_db_deserialize` dispatches on the version number read from the header (currently only `case 1` is implemented). 
  This means the file format can evolve, a v2 layout can be added alongside v1's reader, without breaking the ability to load files written by older versions of the library. 
  This is the same reasoning that motivates having a versioned CMake project: forward compatibility has to be designed in from the first release, not retrofitted once files already exist in the wild.
- **CRC32 checksum over the whole payload**, verified before any deserialization logic runs. 
  If the checksum doesn't match, `sevdb_db_deserialize` fails fast and returns `NULL` rather than attempting to parse a possibly-corrupted structure, corruption is treated as "no valid database," not "best effort partial load."
- **Atomic write via temp-file + rename.** `sevdb_db_serialize` writes to a `mkstemp`-generated temporary file in the same directory, and only `rename()`s it over the destination path after the full write (including checksum) has succeeded and been flushed. 
  Since `rename()` is atomic on POSIX filesystems, a process crash or power loss mid-write can never leave a half-written file at the destination path, the destination is either the old complete file or the new complete file, never a corrupt hybrid. This is why `persistence.c` currently depends on POSIX (`mkstemp`, `unistd.h`) rather than pure C11/C17, standard C has no atomic rename-on-commit primitive (see Known Limitations).

## Error-handling convention

sevdb distinguishes two categories of failure, and handles each differently rather than applying one rule everywhere:

1. **Runtime/data failures**: conditions a correctly-written caller can hit during normal use: allocation failure, a full priority queue, a missing ID, a corrupted file. 
   These are defended against explicitly and communicated via a return code (`false`, `NULL`, or a sentinel like `-1` for `pq_get_count`).
   The caller is expected to check the result.
2. **Precondition violations**: passing a pointer to an object that should already be valid (e.g. calling `sevdb_vector_get_id(NULL)`). 
   These are programmer bugs, not recoverable runtime conditions, and are guarded with
   `assert()` rather than a sentinel return value. A sentinel here would be actively misleading in some cases (`sevdb_vector_get_id` returning `0` on a NULL input is indistinguishable from a legitimately ID-0 vector), so pretending it's a normal "checkable" failure would produce silently wrong answers rather than a loud, debuggable failure.

`_destroy` functions are the one deliberate exception to the assert rule: they follow the standard C idiom (mirroring `free(NULL)`) of being safe no-ops on `NULL`, since cleanup code frequently needs to handle "might not have been allocated" without extra branching at every call site.

## Testing strategy

Tests are split into suites by concern (`internal_test_suite`, `distance_test_suite`, `priority_queue_test_suite`), mirroring the module split in `src/`, plus a standalone `benchmark` binary that isn't part of `ctest` and is run manually to sanity-check search performance at scale (100k vectors, 1536 dimensions a realistic modern embedding size). Persistence tests specifically exercise the serialize/deserialize round trip under several conditions (empty database, high-dimension vectors, post-removal state) to catch bugs that only manifest across a save/load boundary rather than in a single in-memory session.

## Known limitations and future directions

These are open, acknowledged tradeoffs tracked here so the reasoning isn't lost, and expanded on as they're addressed:

- **Linear search is O(n) per query.** Fine at the current scale (see the benchmark), but the natural next step for larger datasets is an approximate nearest-neighbor structure (HNSW is the leading candidate per the devlogs). 
  The `pqueue` abstraction was written to stay reusable for that: whatever candidate-generation strategy replaces the full linear scan can still feed results through the same bounded top-K heap.
- **Dense array storage means O(n) ID lookup/removal.** A hash map keyed by ID is the likely fix, deferred until it's clearly the bottleneck.
- **No concurrency support.** All structures assume single-threaded access.
  Multithreading (either parallelizing the linear scan across the vector array, or supporting concurrent readers) is a planned enhancement once the single-threaded correctness story is solid.
- **Persistence depends on POSIX APIs** (`mkstemp`, `unistd.h`) for atomic writes. 
  Standard C has no equivalent primitive, so portability to non-POSIX targets (e.g. native Windows) would require an abstraction layer over the temp-file-and-rename logic.
