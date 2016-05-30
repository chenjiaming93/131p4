#!/bin/bash

# Running './check.sh' will not rebuild your project, but './check.sh .' will

RED='\033[0;31m'
GREEN='\033[0;32m'
YELL='\033[0;33m' 
NC='\033[0m' # Default Color

TA="cse131_testcases"  # name of directory containing TA's test cases


dir=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
cd $dir  # This should allow you to call this script from anywhere




# If glc is missing or argument is specified, rebuild project
if [ ! -f ../glc ] || [ $# -eq 1 ]; then
  printf "$YELL Rebuilding project...\n $NC"
  (cd .. && make clean && make -j8)
fi


if [ ! -f ../glc ]; then
  printf "$RED Unable to make project\n $NC"
  exit 1
fi


rm *.bc 2> /dev/null








function runTest {
        path=$1
        glsl=$2
        prefix=$3
        fbname=${glsl%%.*}
        bc=${fbname}.bc
        out=${fbname}.out
        

        printf "${NC}${prefix}Test case %s: " $fbname
                 
        # glc
        eval $path/glc < $glsl > $bc 2> /dev/null        
        if [ $? -ne 0 ]; then 
                printf "$YELL\nglc exited with error status\n"
                printf "run  ../glc < $glsl  for more info\n"
                continue
        fi

        # gli
        eval $path/gli $bc &> /dev/null
        if [ $? -ne 0 ]; then
                printf "$YELL\ngli exited with error status\n"
                printf "run  ../gli $bc  for more info\n"
                continue
        fi



        # Print Result
        if diff <(eval $path/gli $bc 2> /dev/null) $out ; then
                printf "$GREEN PASS\n"
        else
                printf "$RED FAIL\n"  
        fi
}






for glsl in *.glsl; do
    runTest .. $glsl ""
done

if [ -d $TA ]; then
    cd $TA
    for glsl in *.glsl; do
        runTest ../.. $glsl "Official " 
    done
else 
    printf "$YELL Could not find directory: $TA\n"
fi






printf $NC
