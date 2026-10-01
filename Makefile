# make – sukompiliuoja (build/muhhash, build/tests, build/experiments)
# make test – golden v0.1 / v0.2 ir pasukimo testai
# make run – visi eksperimentai v0.1 ir v0.2 (rašo results/)
# make clean – ištrina build/

BUILD = build

.PHONY: all test run clean

all:
	cmake -S . -B $(BUILD)
	cmake --build $(BUILD)

test: all
	ctest --test-dir $(BUILD) --output-on-failure

run: all
	scripts/run_all.sh v0.1 v0.2

clean:
	rm -rf $(BUILD)
