# casatables_bench
benchmarks of casacore::tables

The benchmark used a single table with:
- a scalar double TIME column
- an array[3] float UVW column
- an array[N_CHANS, N_POLS] complex DATA column

Table type options:
- `TIME` - only write the `TIME` column
- `UVW` - only write the `UVW` column
- `DATA` - only write the `DATA` column
- `COLUMNWISE` - write all of the rows in one column completely before moving to the next column,
- `ROWWISE` - write one row completely before moving to the next

Write mode options:
- `CELL` - write individual cells with `put`, one at a time
- `CELLS` - write all of the cells for a given timestep in groups using `putColumnCells`
- `COLUMNS` - write an entire column in one go using `putColumn`

## Usage

build with `make release` (or `make debug` for debugging)

```txt
Usage: ./main [-h] [-v] [-V] [-i <iterations>] [-t <tabletype>] [-w <writemode>] [-T <times>] [-B <baselines>] [-C <chans>] [-P <pols>]
  -h: print this help message
  -v: increase verbosity
  -q: decrease verbosity
  -V: validate the table values
  -s: stream junk to the table instead of slicing a pre-allocated array
  -i <iterations>: number of iterations (default: 100 )
  -t <tabletype>: table type (default: COLUMNWISE)
    options: TIME, UVW, DATA, COLUMNWISE, ROWWISE
  -w <writemode>: write mode (default: CELL)
    options: CELL, CELLS, COLUMN
  -T <times>: number of times (default: 12)
  -B <baselines>: number of baselines (default: 8256)
  -C <chans>: number of channels (default: 768)
  -P <pols>: number of polarizations (default: 4)
```

## Results

1000 iterations, `nTimes=12, nBls=8256, nChs=768, nPols=4`

| table type | write mode | user | system | real |
|------------|------------|------|--------|------|
| rowwise | cell | 439.83 | 1118.57 | 1562.98 |
| columnwise | column | 582.01 | 3180.09 | 3773.46 |
| rowwise | cells | 582.35 | 3224.14 | 3822.05 |
| columnwise | cells | 599.05 | 3557.64 | 4170.86 |
| columnwise | cell | 740.21 | 4207.47 | 4965.69 |

raw output of `make bench`

```txt
./main -i 1000 -t columnwise -w cell
nTimes=12, nBls=8256, nChs=768, nPols=4, tableType=COLUMNWISE, writeMode=CELL, iterations=1000
user:   740.21
system: 4207.47
real:   4965.69
./main -i 1000 -t columnwise -w cells
nTimes=12, nBls=8256, nChs=768, nPols=4, tableType=COLUMNWISE, writeMode=CELLS, iterations=1000
user:   599.05
system: 3557.64
real:   4170.86
./main -i 1000 -t columnwise -w column
nTimes=12, nBls=8256, nChs=768, nPols=4, tableType=COLUMNWISE, writeMode=COLUMN, iterations=1000
user:   582.01
system: 3180.09
real:   3773.46
./main -i 1000 -t rowwise -w cell
nTimes=12, nBls=8256, nChs=768, nPols=4, tableType=ROWWISE, writeMode=CELL, iterations=1000
user:   439.83
system: 1118.57
real:   1562.98
./main -i 1000 -t rowwise -w cells
nTimes=12, nBls=8256, nChs=768, nPols=4, tableType=ROWWISE, writeMode=CELLS, iterations=1000
user:   582.35
system: 3224.14
real:   3822.05
```

## Tableinfo

ms created with Cotter: `make tableinfo && ./tableinfo <table>.ms`

```
Number of rows: 122016
Number of columns: 23
F:.isFixedShape(), S:.isScalar(), A:.isArray(), T:.isTable(),
D:.options()&Direct, U:.options()&Undefined
Name            F S A T D U
UVW             F   A   D
FLAG                A
FLAG_CATEGORY       A
WEIGHT              A
SIGMA               A
ANTENNA1        F S
ANTENNA2        F S
ARRAY_ID        F S
DATA_DESC_ID    F S
EXPOSURE        F S
FEED1           F S
FEED2           F S
FIELD_ID        F S
FLAG_ROW        F S
INTERVAL        F S
OBSERVATION_ID  F S
PROCESSOR_ID    F S
SCAN_NUMBER     F S
STATE_ID        F S
TIME            F S
TIME_CENTROID   F S
DATA            F   A
WEIGHT_SPECTRUM F   A
```