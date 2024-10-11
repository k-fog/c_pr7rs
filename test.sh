#!/bin/sh

assert () {
    OUTPUT=`./pr7rs "$1"`
    if [ $OUTPUT = $2 ]; then
        echo $1 '==>' $OUTPUT
    else
        echo $2 expected, but got $OUTPUT
        echo 'FAILED'
        exit
    fi
}

assert '(+ 1 2)' 3
assert '(+ 3 2 1)' 6
assert '(- 1 2)' -1
assert '(- 3 2 1)' 0
assert '(* 1 2)' 2
assert '(* 3 2 1)' 6
assert '(if #t 0 1)' 0
assert '(if #f 0 1)' 1
assert "((if #t '+ '*) 3 5)" 8

echo 'ALL TESTS PASSED'