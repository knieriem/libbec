SRC=\
	calstats_copy.c\
	calstats.h\

TARG=calstatstest

all: test

test: build
	./$(TARG) > test.output
	cmp exp.output test.output

build: $(TARG)

$(TARG): go.mod $(SRC) caltest.go
	go build

clean:
	rm -f $(TARG)
	rm -f $(SRC)
	rm -f go.mod
	rm -f test.output

go.mod:
	go mod init example.com/calstatstest
	go mod tidy

%_copy.c: ../%.c
	cat < $< > $@

%.h: ../%.h
	cat < $< > $@


.PHONY: \
	all\
	build\
	test\
