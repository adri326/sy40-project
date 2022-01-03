make -j --always-make > /dev/null
for n in `seq 40`; do
    ./build/sy40_project | grep "=>" | wc -l
done
