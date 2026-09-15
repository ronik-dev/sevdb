IMAGE := sevdb-dev

build:
	docker compose run --rm dev cmake -S . -B /build
	docker compose run --rm dev cmake --build /build

test: build
	docker compose run --rm dev ctest --test-dir /build --output-on-failure

clean:
	docker compose down -v

shell:
	docker compose run --rm dev bash
