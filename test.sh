#!/bin/sh

assert() {
    OUTPUT=`./pr7rs "$1"`
    EXIT_CODE=$?
    if [ "$OUTPUT" = "$2" ] && [ $EXIT_CODE -eq 0 ]; then
        echo $1 '==>' $OUTPUT
        return
    elif [ $EXIT_CODE -ne 0 ]; then
        echo "\e[31mERROR: Program terminated unexpectedly."
    else
        echo "\e[31mERROR: $2 expected, but got $OUTPUT"
    fi
    exit
}

assert '(+ 1 2)' 3
assert '(+ 3 2 1)' 6
assert '(- 1 2)' -1
assert '(- 3 2 1)' 0
assert '(* 1 2)' 2
assert '(* 3 2 1)' 6
assert '(if #t 0 1)' 0
assert '(if #f 0 1)' 1
assert '((if #t + *) 3 5)' 8
assert '(quote a)' a
assert '(quote (+ 1 2))' '(+ 1 2)'
assert "'a" a
assert "'(a b c)" '(a b c)'
assert "'()" '()'
assert "'(+ 1 2)" '(+ 1 2)'
assert "'(quote a)" '(quote a)'
assert "''a" '(quote a)'
assert "'((1 2) (3 4 5) 6)" '((1 2) (3 4 5) 6)'
assert '12345' '12345'
assert "'12345" '12345'
assert "'#t" '#t'
assert '#t' '#t'
assert '(define x 10)' 'Undefined'

echo "\e[32mALL TESTS PASSED"