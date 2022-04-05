package main

/*
#cgo CPPFLAGS: -DTESTCALSTATS
#cgo CPPFLAGS: -I.
#include <u.h>
#include "calstats.h"

static Calstatspoint nvmpt[7];

static Calstatsstate state = {
	.cur.calstate= -1,
	.f.npt= 1,
};

Calstats stats = {
	.state= &state,
	.period= 30,
	.msperstep = 60000,

	.pt= &nvmpt[0],
	.nptmax= 7,
};
*/
import "C"

import (
	"flag"
	"fmt"
	"log"
)

type step struct {
	state int
	dur   int
}

const (
	Calibrated = C.Calibrated - 1
)

var steps = []step{
	{0, 10},
	{1, 20},
	{2, 5},
	{Calibrated, 100},
	{2, 10},
	{Calibrated, 20},
	{1, 5},
	{2, 80},
	{Calibrated, 100},
	{0, 1},
	{0, 10},
	{Calibrated, 40},
	{2, 220},
	{Calibrated, 10},
	{2, 5},
	{Calibrated, 5},
	{2, 5},
	{Calibrated, 10},
	{1, 180},
}

var steps2 = []step{
	{1, 10},
	{2, 10},
	{Calibrated, 50},
	{2, 10},
	{Calibrated, 50},
	{2, 10},
	{Calibrated, 50},
}

var cflag = flag.Bool("C", false, "enable debug output from C module")

const pctmul = 100

func main() {
	flag.Parse()

	stats := &Stats{c: &C.stats}
	C.calstatsinit(stats.c)
	stats.dosteps(steps, false)
	stats.dosteps(steps, false)
	stats.dosteps(steps2, false)
}

type Stats struct {
	c    *C.Calstats
	ncal int
}

func (st *Stats) dosteps(steps []step, verbose bool) {
	var r C.Calstatsregs

	for i, step := range steps {
		for j := 0; j < step.dur; j++ {
			C.calstatsupdate(st.c, C.int(step.state))
			if verbose {
				fmt.Printf("\tflt:\t%v\n", st.c.state.f)
			}
			C.calstatsregs(&r, st.c, pctmul)
			dpct := 0
			for k, v := range r.val {
				if v.durationpct > 100*pctmul {
					st.dump(&r, i)
					log.Fatal("percentage value > 100: ", i, j, k)
				}
				dpct += int(v.durationpct)
			}
			dpct -= 100 * pctmul
			if dpct < 0 {
				dpct = -dpct
			}
			if dpct > 1 {
				st.dump(&r, i)
				log.Fatal("percentage values not summing up to 100: ", dpct, i, j)
			}
		}
		st.dump(&r, i)
		if step.state == Calibrated {
			st.ncal++
		}
	}
	if st.ncal != int(r.fullcalcnt) {
		log.Fatal("fullcalcnt mismatch: ", st.ncal, r.fullcalcnt)
	}
	fmt.Printf("\n")
}

func (st *Stats) dump(r *C.Calstatsregs, i int) {
	stats := st.c
	f := &stats.state.f
	fmt.Printf("%d: [%d] %v %v\n", i, f.npt, r, f)
}
