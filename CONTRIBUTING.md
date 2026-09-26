# Contributing to sevdb

**sevdb** is primarily a **personal learning project**; a way for me to improve my C skills while building a vector database I use in my own projects. 
That said, issues, pull requests and suggestions for improving the code are always welcome.

## Before contributing
- For anything beyond a small fix (typos, obvious bugs), please open an issue first to discuss the approach.
  This is a learning project and I want to stay hands-on with design decisions 
- Check `docs/devlogs/` for context on current direction and open questions.

## Development setup
This project builds via Docker to guarantee a consistent C17 + Criterion environment:

    make build   # build the project
    make test    # run the test suite
    make shell   # open a dev shell inside the container
    make clean   # clean build artifacts

## Code style
- C17, built with `-std=c17`.
- Header guards: `#pragma once`.
- Public API lives in `include/sevdb/`; internal details stay in `src/` behind opaque pointers (see `sevdb_private.h`).
- Error-handling convention: functions that can fail under normal, correct use (allocation failures, missing IDs, full containers) return a failure code and must be checked by the caller. 
  Functions that take a pointer to an object that should already be valid (e.g. `sevdb_vector_get_id`) assert non-NULL: passing NULL there is a caller bug, not a runtime condition.

## Tests
New functionality should come with Criterion tests in `tests/`. Run `make test` before opening a PR.

## Commit messages
Use your favourite format, as long as it describes what changed.
