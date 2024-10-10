#!/bin/sh

assert () {
    ./pr7rs $1
    OUTPUT=$?
    if [ $OUTPUT = $2 ]; then
        echo "$1 ==> $OUTPUT"
    else
        echo "ERROR"
    fi
}

assert "'foo" "'foo"
assert '(+ 1 2)' 3
assert '((if #t '+ '*) 3 5)' 8
