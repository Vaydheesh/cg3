mkdir -p gcov
rm -f gcov/*.gcov
rm -f gcov/*.gcda
rm -f gcov/*.gcno
rm -f gcov/vislcg3
cd gcov
g++ -Wall -Wextra -Wno-deprecated -O -g3 -fprofile-arcs -ftest-coverage -ffor-scope -licuio -licuuc $(ls -1 ../src/*.cpp | egrep -v '/test_' | egrep -v '/cg_' | grep -v Apertium) -o vislcg3
cd ..
mkdir -p gprof
rm -f gprof/vislcg3
rm -f gprof/gmon.out
cd gprof
g++ -pg -Wall -Wextra -Wno-deprecated -O -g3 -ffor-scope -licuio -licuuc $(ls -1 ../src/*.cpp | egrep -v '/test_' | egrep -v '/cg_' | grep -v Apertium) -o vislcg3
