#!/bin/bash

set -bm
trap 'if [[ $? -eq 139 ]]; then echo "segfault $testbasename!" >> $testbasename.out; fi' CHLD

if (! [ -d public_samples ]); then
        echo "public_samples folder not found. Creating one for you"
        echo "Copy all .glsl .dat and .out test file inside this folder"
        mkdir public_samples
        exit 1
fi

echo "Cleaning Up..."
make clean &>/dev/null

echo "Compiling ..."
make &>/dev/null

echo "Compiling Done" 

if cd public_samples; then
	cp ../glc ./
        chmod +x glc

	if (! [ -f ../gli ]); then 
		echo "gli is not present"
		exit 1
	fi 	
	cp ../gli ./
	chmod +x gli	
else 
	echo "Could not change diretcory to samples ..Quiting 	"
	exit 1
fi	

if [ -f glc ]; then
        for testname in $PWD/*.glsl
        do
                testid=$(basename $testname)
                testbasename=${testid%.glsl}
                rm -rf "$testbasename".ll
		rm -rf "$testbasename".bc
                ./glc <$testname > $testbasename.bc
		llvm-dis $testbasename.bc
                ./gli $testbasename.bc > $testbasename.myout
        done

        for testname in $PWD/*.out
        do
                testid=$(basename $testname)
                testbasename=${testid%.out}
                if cmp -s "$testbasename.myout" "$testbasename.out"
                then
                        echo "$testbasename Passed..Cleaning debug files for this test"
			rm $testbasename.ll
			rm $testbasename.bc
                else
                        echo "$testbasename Failed"
                fi
        done
	rm -rf gli
	rm -rf glc	
else
        echo "Code did not compile"
fi
