# casatables_bench
benchmarks of casacore::tables

## Results

```
❯ ./main -i 1000 -t time -w cell -T 128 -B 128 -C 128 -P 4
iterations: 1000 tabletype: TIME writemode: CELL
user:   5.27
system: 30.61
real:   36.02
❯ ./main -i 1000 -t time -w cells -T 128 -B 128 -C 128 -P 4
iterations: 1000 tabletype: TIME writemode: CELLS
user:   4.97
system: 29.58
real:   34.66
❯ ./main -i 1000 -t time -w column -T 128 -B 128 -C 128 -P 4
iterations: 1000 tabletype: TIME writemode: COLUMN
user:   3.87
system: 29.11
real:   33.06
```