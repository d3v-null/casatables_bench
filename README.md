# casatables_bench
benchmarks of casacore::tables

## Results

`make bench`

```
./main -i 100 -t columnwise -w cell
nTimes=12, nBls=8256, nChs=768, nPols=4, tableType=COLUMNWISE, writeMode=CELL
iterations: 100
user:   78.01
system: 360.19
real:   439.95
./main -i 100 -t columnwise -w cells
nTimes=12, nBls=8256, nChs=768, nPols=4, tableType=COLUMNWISE, writeMode=CELLS
iterations: 100
user:   61.18
system: 357.02
real:   419.42
./main -i 100 -t columnwise -w column
nTimes=12, nBls=8256, nChs=768, nPols=4, tableType=COLUMNWISE, writeMode=COLUMN
iterations: 100
user:   60.14
system: 358.23
real:   419.87
```