#!/bin/bash
for i in $(ls *.glsl *.dat *.out)
do
echo "begin to modify the fileformat"
vi +":set fileformat=unix" +":wq" $i
done
