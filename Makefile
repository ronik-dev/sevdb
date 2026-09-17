IMAGE := sevdb-dev

build:
	docker compose run --rm dev cmake -S . -B /build
	docker compose run --rm dev cmake --build /build

test: build
	docker compose run --rm dev sh -c "/build/internal_test_suite && /build/distance_test_suite"

clean:
	docker compose down -v

shell:
	docker compose run --rm dev bash
