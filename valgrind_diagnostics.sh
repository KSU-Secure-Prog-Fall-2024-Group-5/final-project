#! /bin/sh

cat > logappend_tmp.batch << STUFF
-K   secret -T 1 -A -E John log2


-K secret   -T 2 -A -R 0 -E John log2
-K  secret -T 3 -A -G James log2
-K secret  -T 4 -A -R 0 -G James log2
-K   secret -T 1 -A -E John log2 

  -K secret   -T 2 -A -R 0 -E John log2
-K  secret -T 3 -A -G James log2
-K secret  -T 4 -A -R 0 -G James log2
-K secret   -T 2 -A -R 0 -E John log2
-K  secret -T 3 -A -G James log2
-K secret  -T 4 -A -R 0 -G James log2
-K   secret -T 1 -A -E John log2 

  -K secret   -T 2 -A -R 0 -E John log2
-K  secret -T 3 -A -G James log2
-K secret  -T 4 -A -R 0 -G James log2
-K secret   -T 2 -A -R 0 -E John log2
-K  secret -T 3 -A -G James log2
-K secret  -T 4 -A -R 0 -G James log2
-K   secret -T 1 -A -E John log2 

  -K secret   -T 2 -A -R 0 -E John log2
-K  secret -T 3 -A -G James log2
-K secret  -T 4 -A -R 0 -G James log2
-K secret   -T 2 -A -R 0 -E John log2
-K  secret -T 3 -A -G James log2
-K secret  -T 4 -A -R 0 -G James log2
-K   secret -T 1 -A -E John log2 

  -K secret   -T 2 -A -R 0 -E John log2
-K  secret -T 3 -A -G Jameees log2
STUFF

rm log2
valgrind --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=2 --track-fds=yes ./logappend -B logappend_tmp.batch
valgrind --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=5 --track-fds=yes ./logread -K secret -S log2
rm logappend_tmp.batch log2