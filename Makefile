IMAGE := sevdb-dev

build:
	docker compose run --rm dev cmake -S . -DSEVDB_BUILD_TESTS=OFF -B /build 
	docker compose run --rm dev cmake --build /build

build-tests:
	docker compose run --rm dev cmake -S . -DSEVDB_BUILD_TESTS=ON -B /build
	docker compose run --rm dev cmake --build /build

benchmark:
	docker compose run --rm dev cmake -S . -DSEVDB_BUILD_TESTS=ON -B /build 
	docker compose run --rm dev cmake --build /build --target benchmark
	docker compose run --rm dev /build/benchmark

build-release:
	docker compose run --rm dev cmake -S . -DSEVDB_BUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=Release -B /build-release 
	docker compose run --rm dev cmake --build /build-release

benchmark-release:
	docker compose run --rm dev cmake -S . -DSEVDB_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release -B /build-release
	docker compose run --rm dev cmake --build /build-release --target benchmark
	docker compose run --rm dev /build-release/benchmark

test: build-tests
	docker compose run --rm dev ctest --test-dir /build -VV

clean:
	docker compose down -v

shell:
	docker compose run --rm dev bash
