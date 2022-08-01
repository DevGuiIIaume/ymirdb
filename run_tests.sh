#!/bin/bash

# USYD CODE CITATION ACKNOWLEDGEMENT
# I declare that the scaffold to the following code has been taken from
# my submission to the 2020 INFO1110 "Acorn" Assignment

# Trigger all the test cases with this script
echo "##########################"
echo "### Running e2e tests! ###"
echo "##########################"
echo ""

# Initialise counting variable
count=0

# Flag for whether valgrind/gcovr is to be run or not
valgrind=false
gcovr=false

if [ "-valgrind" == "$1" ]; then
    valgrind=true
elif [ "-gcovr" == "$1" ]; then
    gcovr=true
fi

if [ true = "$valgrind" ]; then
    echo "Running valgrind"
    echo ""

    gcc ymirdb.c -o bin_ymirdb -Wall -Wvla -Werror -g -std=gnu11 -Werror=format-security
elif [ true = "$gcovr" ]; then
    echo "Running gcovr"
    echo ""

    # Remove gcvor files
    rm *.gcda
    rm *.gcno
    rm *.css
    rm *.html

    gcc ymirdb.c -o bin_ymirdb -fsanitize=address -Wall -Wvla -Werror -g -std=gnu11 -Werror=format-security -fprofile-arcs -ftest-coverage -fPIC
else
    gcc ymirdb.c -o bin_ymirdb -fsanitize=address -Wall -Wvla -Werror -g -std=gnu11 -Werror=format-security
fi

# Assume all ".in" and ".out" files are located in a separate `tests` directory
for folder in `ls -d tests/*/ | sort -V`; do

    name=$(basename "$folder")

    echo Running $name.

    in_file=tests/$name/*.in
    expected_file=tests/$name/*.out

    if [ true = "$valgrind" ]; then
        # Quiet leak check (do not print result unless there is an error)
        valgrind --leak-check=full -q ./bin_ymirdb < $in_file | diff - $expected_file || echo "Test $name: failed!"
    else
        ./bin_ymirdb < $in_file | diff - $expected_file || echo "Test $name: failed!"
    fi

    count=$((count+1))
done

echo ""
echo "Finished running $count tests!"
echo ""

if [ true == "$gcovr" ]; then
    gcovr -r . --html --html-details -o code_coverage.html
fi
