#!/bin/bash

echo "=== Testing Simple Commands ==="
echo "PWD:"; pwd
echo "WHOAMI:"; whoami
echo

echo "=== Testing Output Redirection > ==="
echo "Hello Output Redirection" > test_out.txt
cat test_out.txt
echo

echo "=== Testing Append Redirection >> ==="
echo "Line 1" > test_append.txt
echo "Line 2" >> test_append.txt
cat test_append.txt
echo

echo "=== Testing Input Redirection < ==="
echo "This is a test input" > input.txt
cat < input.txt
echo

echo "=== Testing Error Redirection 2> ==="
ls nonexistentfile 2> error.txt
echo "Contents of error.txt:"
cat error.txt
echo

echo "=== Testing Pipes | ==="
echo -e "c\nb\na" > sortme.txt
cat sortme.txt | sort
echo

echo "=== Testing Multiple Pipes ==="
echo "abc" | tr a-z A-Z | rev
echo

echo "=== Testing Background Execution & ==="
sleep 2 &
echo "This should appear before sleep ends"
