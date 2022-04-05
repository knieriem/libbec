package main

import (
	"C"
	"bytes"
	"fmt"
	"unsafe"
)

//export calstatstest_print
func calstatstest_print(s *C.uchar, v int) {
	if !*cflag {
		return
	}
	buf := unsafe.Slice((*byte)(s), 256)

	i := bytes.IndexByte([]byte(buf), 0)
	if i != -1 {
		buf = buf[:i]
	}
	fmt.Printf("\tC: %v: %v\n", string(buf), v)
}
