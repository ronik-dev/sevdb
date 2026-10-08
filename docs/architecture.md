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

## Data structure: Packed vector array with a Hash Index

`sevdb_database` uses two complementary data structures:

```c
struct sevdb_database {
    uint32_t capacity;
    uint32_t count;
    sevdb_vector **vectors;
    hashmap *vector_id_map;
};
```

The `vectors` array is the primary storage for vector objects. It is allocated to `capacity` entries, while `count` records how many vectors are currently stored. The database maintains a **packed-array invariant**: all stored vectors occupy the range `[0, count)`, with no `NULL` holes between active entries.
The second structure, `vector_id_map`, is a hash map from a vector's unique `uint32_t` ID to its current index in the `vectors` array.
This separation gives the database two useful properties:
- The vector array provides compact, contiguous storage of vector pointers and makes scanning all vectors straightforward.
- The hash map provides direct lookup of a vector's array position by ID without scanning the entire database.

### ID lookup

`sevdb_db_get_vector_by_id` first queries `vector_id_map` using the vector ID. If the key exists, the stored value is interpreted as an index into `vectors`, and the vector at that position is returned after verifying that its ID still matches.
Under normal operation, hash-map lookup is approximately **O(1)** average time, compared with the O(n) lookup that would be required by scanning the vector array.
The hash map therefore acts as an **index**, rather than being the primary owner of vector objects. It stores only the relationship:

```text
vector ID -> position in vectors[]
```

The actual vector remains owned by the database's vector array.

### Removal and packed-array maintenance

Removing a vector does not leave a hole in the array. Instead, `sevdb_db_remove_vector_by_id` performs a **swap-with-last** operation:

1. Find the vector's array index through `vector_id_map`.
2. Remove its ID from the hash map.
3. If the vector is not already the last element, move the last vector into the removed vector's position.
4. Update the moved vector's hash-map entry to its new index.
5. Clear the old last slot.
6. Decrement `count`.

For example:

```text
Before removal:

vectors:
[ A ][ B ][ C ][ D ]

map:
A -> 0
B -> 1
C -> 2
D -> 3

Remove B:

vectors:
[ A ][ D ][ C ][ NULL ]

map:
A -> 0
C -> 2
D -> 1
```

This makes removal **O(1) average time** for the hash-map lookup plus constant-time array manipulation. The tradeoff is that vector positions are not stable: removing one vector can change the array index of another vector. Code outside the database should therefore never treat an array index as a persistent vector identifier.

### Insertion

`sevdb_db_push_vector` appends a vector at `vectors[count]` and inserts its ID and resulting index into the hash map.

Insertion is rejected when:
- the database is full;
- the vector pointer is `NULL`; or
- another vector already has the same ID.

The operation is performed transactionally with respect to the two data structures: the vector is first placed into the array and `count` is incremented, but if the hash-map insertion fails, the array slot and count are rolled back.
This is important because the vector array and hash index must remain consistent. A vector present in the array without a corresponding hash-map entry would make ID lookup incorrect.

### Capacity growth

`sevdb_db_increase_capacity` grows only the vector-pointer array. Existing vector objects are not moved because the array contains pointers rather than the vector objects themselves.
The new region of the pointer array is explicitly zeroed, preserving the `NULL` representation for unused capacity.
Capacity growth is one-directional: the database can increase its capacity but does not currently shrink it.
The hash map is independent of this operation. Its own capacity is managed internally by `hm_grow`, which rehashes existing entries when its load factor reaches the configured threshold.

### Hash-map implementation

The ID index uses an open-addressed hash table with linear probing.
Each hash-map slot has one of three states:

```text
EMPTY
USED
DELETED
```

The hash function currently uses the integer ID directly:

```c
key % capacity
```

Because the vector IDs are already integer keys, this keeps the implementation simple.
Collisions are resolved through linear probing. Deleted entries are marked `DELETED` rather than reset to `EMPTY`, because an `EMPTY` slot would terminate a subsequent probe sequence and could make keys that occur later in the sequence unreachable.
When the hash map becomes sufficiently full, `hm_grow` allocates a larger table and re-inserts all active entries using the new capacity. Rehashing is necessary because changing the table capacity changes the result of `key % capacity`.
The hash map therefore gives the database approximately **O(1) average ID lookup, insertion, and removal**, while the packed vector array provides efficient sequential traversal for operations such as similarity search.

## Similarity search: linear scan with bounded top-K heap

`sevdb_db_search_k_similar_vectors` performs an exhaustive linear search over the vectors currently stored in the database.
For each candidate with the same dimensionality as the query vector, the database calculates cosine similarity using `get_cosine_similarity`. The search does not allocate a result structure proportional to the database size. Instead, it maintains a fixed-capacity priority queue containing at most `k` candidates.
For cosine similarity, larger scores are better. The priority queue is therefore configured as a **min-heap**, making the root the worst candidate currently retained in the top-K set.
The algorithm is:

```text
for every stored vector:
    skip incompatible dimensions

    calculate cosine similarity

    if fewer than K results are stored:
        insert candidate
    else if candidate is better than the worst stored result:
        remove worst result
        insert candidate
```

This produces:

- **Time:** O(n log k)
- **Additional memory:** O(k)
- **Vector scoring:** O(n · d), where `d` is the vector dimensionality

The distance calculation is generally the dominant operation because every compatible vector must be examined.

The priority queue is deliberately generic. Its comparison function determines whether it behaves as a min-heap or max-heap, allowing the same implementation to support metrics where either smaller or larger scores represent better matches.

For cosine similarity, `sevdb_db_search_k_similar_vectors` uses:

```c
static bool is_min_heap(float a, float b) {
    return a < b;
}
```

This may initially appear counterintuitive because the search wants the highest cosine similarities. The min-heap is intentional: the smallest score among the retained top-K candidates is placed at the root so it can be efficiently identified and replaced when a better candidate is found.

The current implementation iterates through the allocated `capacity` of the pointer array and ignores `NULL` entries. Because the database now maintains a packed-array invariant, this could be simplified to iterate over `[0, count)` instead. Doing so would make the relationship between the data structure invariant and the search implementation explicit and avoid inspecting unused capacity.

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

These are open tradeoffs that are intentionally documented so that future changes preserve the reasoning behind the current architecture.
- **Similarity search is O(n log k).**  
  The ID index makes individual vector lookup approximately O(1) on average, but nearest-neighbor search still requires examining every compatible vector. At larger dataset sizes, the natural next step is an approximate nearest-neighbor structure such as HNSW.
- **The hash index consumes additional memory.**  
  Each vector is represented both by its object in the vector array and by an entry in `vector_id_map`. This is worthwhile because ID lookup and removal are approximately O(1) on average, but it introduces additional memory overhead and requires the array and index to remain synchronized.
- **Vector positions are not stable.**  
  Removal uses swap-with-last compaction. As a result, deleting one vector can change the array index associated with another vector. The hash map is therefore the authoritative mechanism for resolving IDs to positions; callers should not depend on internal array positions.
- **Similarity search is currently exhaustive.**  
  The existing priority queue is reusable as the top-K result-selection mechanism, so an approximate candidate-generation structure could be introduced later without necessarily changing the result-ranking interface.
- **No concurrency support.**  
  The database, hash map, and vector storage currently assume single-threaded access. Concurrent reads or writes would require synchronization and a clearly defined ownership/thread-safety model.
- **Persistence depends on POSIX APIs.**  
  Atomic serialization currently relies on APIs such as `mkstemp`, `unistd.h`, and `rename()`. Portability to native Windows or other non-POSIX environments would require an abstraction around temporary-file creation and atomic replacement.
- **Capacity only grows.**  
  The database currently supports explicit capacity increases but has no shrink-to-fit mechanism. This keeps the memory model simple but can leave unused pointer capacity after large databases are reduced in size.
- **Vector dimensionality is checked during search.**  
  Vectors with dimensions different from the query vector are skipped rather than rejected globally. This allows a database to contain vectors of different dimensionalities, but means a query only considers compatible vectors.
