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

Calstats stats2 = {
	.state= &state,
	.period= 28800,
	.msperstep = 3000,

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

	fmt.Println("* basic test")
	stats := newStats(&C.stats)
	stats.dosteps(steps, false)
	stats.dosteps(steps, false)
	stats.dosteps(steps2, false)

	var r C.Calstatsregs
	fmt.Println("* testing counter overflow protection")
	stats = newStats(&C.stats2)
	stats.reset()
	for i := 0; i < 256+1; i++ {
		stats.update(3)
		stats.update(1 + (i & 1))
	}
	stats.setupRegs(&r, 0)
	stats.dump(&r, 0)

	// continue using stats, not resetting them
	for i := 0; i < 28800-257; i++ {
		stats.update(3)
		if i == 10000 || i == 20000 {
			stats.update(2)
		}
	}
	stats.setupRegs(&r, 0)
	stats.dump(&r, 0)

	for i := 0; i < 28800-257-2-2; i++ {
		stats.update(3)
		if i == 10000 || i == 20000 {
			stats.update(2)
		}
	}
	stats.setupRegs(&r, 0)
	stats.dump(&r, 0)
}

type Stats struct {
	c    *C.Calstats
	ncal int
}

func newStats(c *C.Calstats) *Stats {
	st := new(Stats)
	st.c = c
	C.calstatsinit(c)
	return st
}

func (st *Stats) update(calState int) {
	C.calstatsupdate(st.c, C.int(calState))
}

func (st *Stats) setupRegs(r *C.Calstatsregs, i int) {
	C.calstatsregs(r, st.c, pctmul)

	dpct := 0
	for k, v := range r.val {
		if v.durationpct > 100*pctmul {
			st.dump(r, i)
			log.Fatal("percentage value > 100: ", i, k)
		}
		dpct += int(v.durationpct)
	}
	dpct -= 100 * pctmul
	if dpct < 0 {
		dpct = -dpct
	}
	if dpct > 1 {
		st.dump(r, i)
		log.Fatal("percentage values not summing up to 100: ", dpct, i)
	}
}

func (st *Stats) reset() {
	C.calstatsreset(st.c)
}

func (st *Stats) dosteps(steps []step, verbose bool) {
	var r C.Calstatsregs

	for i, step := range steps {
		for j := 0; j < step.dur; j++ {
			st.update(step.state)
			if verbose {
				fmt.Printf("\tflt:\t%v\n", st.c.state.f)
			}
			st.setupRegs(&r, i)
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
	fmt.Printf("%d: [%d] %v %v (%d)\n", i, f.npt, r, f, stats.state.elapsed)
}
