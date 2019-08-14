all:
	make -C ties 
	make -C rng

test:
	make test -C ties
	make test -C rng
	python3 test.py $(TEST_ARGS)

clean:
	make clean -C ties
	make clean -C rng
	rm -rf test_output/
