export ITERS=100
set -ex
./main -i $ITERS -t time -w cell
./main -i $ITERS -t time -w cells
./main -i $ITERS -t time -w column