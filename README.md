# sevdb
*Small Embedded Vector Database*

#### Goal
This is a personal project that aims to create a simple embedded vector database in C.
This does not intend to be a finished production ready product, as i am more interested in learning how vector databases work and improve my C programming language skills.
The main functionalities implemented include:
- Uploading and deleting vectors using a **Hash Index and Packed Array**, achieving O(1) average lookups and deletions via a swap-with-last strategy.
- Fast Top-K retrieval using Cosine Similarity and a **Bounded Min-Heap** to achieve O(n log k) search complexity without allocating proportional memory.
- Full support for multiple vector sizes and dimensions.
- Persistent versioned binary format with **CRC32 checksums** and atomic writes.

This project will not (at least in the short term)
- Provide authentication/authorization to the database or the stored data
- Provide encryption of any kind for the serialized data
- Provide a structured query language, functionalities will be accessible via the program api

#### Build Test and Run
The project relies on [Docker](https://www.docker.com/) to provide a consistent C17 build environment with [CMake](https://cmake.org/) and [Criterion](https://criterion.readthedocs.io/en/master/intro.html)* pre-installed. 
In the future i will provide a build without testing dependencies, but its out of scope for now.

Use the included `Makefile` to interact with the project:
- **Build the project:** `make build`
- **Build the release version:** `make build-release`
- **Run the test suite:** `make test`
- **Run the benchmark (Dev):** `make benchmark`
- **Run the benchmark (Release):** `make benchmark-release`
- **Clean the environment:** `make clean`
- **Open a dev shell:** `make shell`

#### Using sevdb in your project
sevdb is currently consumed via CMake as a subdirectory:

    git submodule add https://github.com/<you>/sevdb third_party/sevdb

    # in your CMakeLists.txt
    add_subdirectory(third_party/sevdb)
    target_link_libraries(your_target PRIVATE sevdb)

Note this pulls in `SEVDB_BUILD_TESTS` defaulting to ON — set it to OFF in your project
to avoid requiring Criterion:

    set(SEVDB_BUILD_TESTS OFF CACHE BOOL "" FORCE)

#### Updates
To stay up to date with the latest changes to the project, take a look at `CHANGELOG.md`

#### Architecture
If you are interested in the project's architecture and the choices behind it, please read `ARCHITECTURE.md`

#### Contributing
If you are interested in contributing but don't know where to start, please read `CONTRIBUTING.md`
