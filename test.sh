#!/bin/sh

$ALL_TESTS_PASSED=true

assert() {
    OUTPUT=`./pr7rs "$1"`
    EXIT_CODE=$?
    if [ "$OUTPUT" = "$2" ] && [ $EXIT_CODE -eq 0 ]; then
        echo $1 '==>' $OUTPUT
        return
    elif [ $EXIT_CODE -ne 0 ]; then
        echo "\e[31mERROR: Program terminated unexpectedly.\e[0m"
    else
        echo "\e[31mERROR: $2 expected, but got $OUTPUT\e[0m"
    fi
    ALL_TESTS_PASSED=false
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

assert '((lambda (x) (- x 10)) 10 )' 0
assert '((lambda (x y) (* x y)) 10 20)' 200

assert '(boolean? #f)' '#t'
assert '(boolean? 0)' '#f'
assert "(boolean? '())" '#f'
assert '(number? 3)' '#t'
assert "(number? '(1))" '#f'
assert '(procedure? car)' '#t'
assert "(procedure? 'car)" '#f'
assert '(procedure? (lambda (x) (* x x)))' '#t'
assert "(procedure? '(lambda (x) (* x x)))" '#f'
assert '(procedure? 0)' '#f'
assert '(null? ())' '#t'
assert '(null? (quote ()))' '#t'
assert '(null? #t)' '#f'
assert '(null? #f)' '#f'
# assert "(pair? '(a . b))" '#t'
assert "(pair? '(a b c))" '#t'
assert "(pair? '())" '#f'

assert "(car '(a b c))" 'a'
assert "(car '((a) b c))" '(a)'
# assert "(car '(1 . 2))" '1'
# assert "(car '())" error
assert "(cdr '((a) b c d))" '(b c d)'
#assert "(cdr '(1 . 2))" '2'

assert "(symbol? 'foo)" '#t' 
assert "(symbol? (car '(a b)))" '#t'
assert "(symbol? 'nil)" '#t'
assert "(symbol? '())" '#f'
assert "(symbol? #f)" '#f'

if $ALL_TESTS_PASSED; then
    echo "\e[32mALL TESTS PASSED"
else
    echo "\e[31mSOME TESTS FAILED"
fi