OPTIMIZED_FLAGS = -O3 -fno-rtti -flto -fno-threadsafe-statics
DEBUG_FLAGS = -O0 -fsanitize=address -lasan
COMMON_PART = -Wall -Wextra -Wpedantic -ggdb src/main.cc -o main --std=c++14

all: clean build-opt tests run-tests

clean:
	rm -f main tests random.txt
build:
	g++ $(COMMON_PART) $(DEBUG_FLAGS)
build-opt:
	g++ $(COMMON_PART) $(OPTIMIZED_FLAGS)
build-clang:
	clang++ $(COMMON_PART) $(DEBUG_FLAGS)
build-opt-clang:
	clang++ $(COMMON_PART) $(OPTIMIZED_FLAGS)
tests:
	g++ -ggdb -O0 src/tests/tests.cc -o tests --std=c++14 $(DEBUG_FLAGS) -lgtest -lpthread
run-tests: tests
	./tests
random.txt:
	Rscript --vanilla ./gen.R >random.txt
