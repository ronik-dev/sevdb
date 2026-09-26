# Changelog
All notable changes to this project will be documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.1.0] - 2026-09-26
### Added
- Vector and database lifecycle management (create/destroy) with a callee-allocates heap model.
- CRUD operations: push, get-by-id, remove-by-id, capacity growth.
- Euclidean distance and cosine similarity metrics.
- Top-K similarity search backed by a binary heap priority queue.
- Binary serialization/deserialization with CRC32 checksums and atomic temp-file writes.
- Criterion-based test suites (internal, distance, priority queue) and a benchmark harness.
