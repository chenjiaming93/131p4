#!/bin/bash
for i in $(ls *.*)
do
echo "begin to modify the fileencoding of $i to utf-8"
vi +":set fileformat=unix" +":wq" $i
done
