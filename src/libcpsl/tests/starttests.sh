#!/bin/sh

# Compile.
cd tests

echo "Compiling unit tests..."
for c in *.c
do
	echo "CC " $c
	$1 $2 $c -o bin/"$c".test ../libcpsl.a
done

echo ""
echo "Executing unit tests..."
for b in bin/*
do
	echo ""
	echo "EXEC  $b"
	if ! $b
	then
		exit 1
	fi
done
