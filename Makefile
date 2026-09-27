IMAGE := sevdb-dev

build:
	docker compose run --rm dev cmake -S . -DSEVDB_BUILD_TESTS=OFF -B /build 
	docker compose run --rm dev cmake --build /build

build-tests:
	docker compose run --rm dev cmake -S . -DSEVDB_BUILD_TESTS=ON -B /build
	docker compose run --rm dev cmake --build /build

test: build-tests
	docker compose run --rm dev ctest --test-dir /build -VV

clean:
	docker compose down -v

shell:
	docker compose run --rm dev bash
