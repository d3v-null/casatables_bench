#include <casacore/tables/Tables.h>
#include <casacore/casa/OS/Directory.h>
#include <casacore/casa/OS/Timer.h>

using namespace casacore;

#define N_TIMES 120
#define N_ANTS 128
#define N_BLS (N_ANTS * (N_ANTS + 1) / 2)
#define N_CHANS (24 * 128)
#define N_POLS 4

// make an enum of table types
typedef enum TableType {
    TIME,
    UVW,
    DATA,
    ALL
} TableType;

std::map<std::string, TableType> tabletype_map = {
    {"time", TIME},
    {"uvw", UVW},
    {"data", DATA},
    {"all", ALL}
};

// enum of write modes
typedef enum WriteMode {
    CELL,
    CELLS,
    COLUMN
} WriteMode;

std::map<std::string, WriteMode> writemode_map = {
    {"cell", CELL},
    {"cells", CELLS},
    {"column", COLUMN}
};

typedef struct SizeInfo {
    int nTimes;
    int nBls;
    int nChs;
    int nPols;
} SizeInfo;

// A table containing:
// - a scalar double TIME column
// - an array[3] float UVW column
// - an array[N_CHANS, N_POLS] complex DATA column
Table setup_table(const String& tableName, SizeInfo size) {
    Directory dir("data");
    if (dir.exists()) {
        dir.remove();
    }
    // from https://casacore.github.io/casacore/group__Tables__module.html#Tables:creation
    // Step1 -- Build the table description.
    TableDesc td("tTableDesc", "1D", TableDesc::Scratch);
    td.comment() = "A table with a single column of double values.";
    td.addColumn (ScalarColumnDesc<Double> ("TIME", ColumnDesc::Direct));
    td.addColumn (ArrayColumnDesc<Float> ("UVW", IPosition(1, 3), ColumnDesc::Direct));
    td.addColumn (ArrayColumnDesc<Complex> ("DATA", IPosition(2, size.nChs, size.nPols), ColumnDesc::Direct));

    SetupNewTable newtab(tableName, td, Table::New);

    Table tab(newtab, size.nTimes * size.nBls);
    return tab;
}

// Synthesize test data for the table
void synthesize_data(Vector<Double>& time, SizeInfo size) { // , Array<Float>& uvw, Array<Complex>& data) {
    indgen(time);
    // indgen(uvw);
    // indgen(data);
}

// fill the time column using the given write mode
void fill_time_col(Table& tab, WriteMode mode, Vector<Double>& time, SizeInfo& size) {
    ScalarColumn<Double> timeCol(tab, "TIME");
    switch (mode) {
        case CELL:
            for (int i = 0; i < (size.nTimes * size.nBls); i++) {
                timeCol.put(i, time[i]);
            }
            break;
        case CELLS:
            for (int i = 0; i < size.nTimes; i++) {
                Slicer chunk( IPosition(1, i * size.nBls), IPosition(1, size.nBls));
                timeCol.putColumnRange(chunk, time(chunk));
            }
            break;
        case COLUMN:
            timeCol.putColumn(time);
            break;
        default:
            printf("todo: fill_time_col");
            break;
    }
}

void fill_uvw_col(Table& tab, WriteMode mode, Array<Float>& uvw, SizeInfo& size) {
    ArrayColumn<Float> uvwCol(tab, "UVW");
    switch (mode) {
        case CELL:
            for (int i = 0; i < (size.nTimes * size.nBls); i++) {
                uvwCol.put(i, uvw(Slicer(IPosition(1, i * 3), IPosition(1, 3))));
            }
            break;
        case COLUMN:
            uvwCol.putColumn(uvw);
            break;
        case CELLS:
        default:
            printf("todo: fill_uvw_col");
            break;
    }
}

int compare_time_col(Table& tab, Vector<Double>& time, SizeInfo& size) {
    ScalarColumn<Double> timeCol(tab, "TIME");
    for (int i = 0; i < (size.nTimes * size.nBls); i++) {
        if (timeCol(i) != time[i]) {
            printf("error: time mismatch in %s at %d: %f != %f", tab.tableName(), i, timeCol(i), time[i]);
            return 1;
        }
    }
    return 0;
}

void usage(char const *argv[]) {
    cout << "Usage: " << argv[0] << "[-h] [-V] [-i <iterations>] [-t <tabletype>] [-w <writemode>]" \
        << " [-T <times>] [-B <baselines>] [-C <chans>] [-P <pols>]\n";
}

int main(int argc, char const *argv[])
{
    // default arg values
    int iterations = 100;
    TableType tabletype = TIME;
    WriteMode writemode = CELL;
    SizeInfo size = {N_TIMES, N_BLS, N_CHANS, N_POLS};
    bool validate = false;

    // parse argv
    int argi = 0;
    while (++argi < argc) {
        // printf("arg %d: %s\n", argi, argv[argi]);
        if(argv[argi][0] == '-') {
            switch(argv[argi][1]) {
                case 'h':
                    usage(argv);
                    return 0;
                case 'V':
                    validate = true;
                    break;
                case 'i':
                    if (++argi < argc) {
                        iterations = atoi(argv[argi]);
                    } else {
                        usage(argv);
                        printf("error: missing iterations argument\n");
                        return 1;
                    }
                    break;
                case 't':
                    if (++argi < argc) {
                        tabletype = tabletype_map[argv[argi]];
                    } else {
                        usage(argv);
                        printf("error: missing tabletype argument\n");
                        return 1;
                    }
                    break;
                case 'w':
                    if (++argi < argc) {
                        writemode = writemode_map[argv[argi]];
                    } else {
                        usage(argv);
                        printf("error: missing writemode argument\n");
                        return 1;
                    }
                    break;
                case 'T':
                    if (++argi < argc) {
                        size.nTimes = atoi(argv[argi]);
                    } else {
                        usage(argv);
                        printf("error: missing times argument\n");
                        return 1;
                    }
                    break;
                case 'B':
                    if (++argi < argc) {
                        size.nBls = atoi(argv[argi]);
                    } else {
                        usage(argv);
                        printf("error: missing baselines argument\n");
                        return 1;
                    }
                    break;
                case 'C':
                    if (++argi < argc) {
                        size.nChs = atoi(argv[argi]);
                    } else {
                        usage(argv);
                        printf("error: missing chans argument\n");
                        return 1;
                    }
                    break;
                case 'P':
                    if (++argi < argc) {
                        size.nPols = atoi(argv[argi]);
                    } else {
                        usage(argv);
                        printf("error: missing pols argument\n");
                        return 1;
                    }
                    break;
                default:
                    printf("unknown option: %s\n", argv[argi]);
                    return 1;
            }
        }
    }

    Vector<Double> times(size.nTimes * size.nBls);
    // Array<Float> uvws(IPosition(size.nTimes * size.nBls, 3));
    // Array<Complex> data(IPosition(size.nTimes * size.nBls, size.nChs, size.nPols));

    synthesize_data(times, size); // , uvws, data);

    switch (tabletype) {
        case TIME: printf("tabletype: TIME "); break;
        case UVW: printf("tabletype: UVW "); break;
        case DATA: printf("tabletype: DATE "); break;
        case ALL: printf("tabletype: ALL "); break;
        default: printf("tabletype: UNKNOWN "); return 1;
    }
    switch (writemode) {
        case CELL: printf("writemode: CELL "); break;
        case CELLS: printf("writemode: CELLS "); break;
        case COLUMN: printf("writemode: COLUMN "); break;
        default: printf("writemode: UNKNOWN "); return 1;
    }

    Table tab = setup_table("table.data", size);

    if (validate) {
        switch (tabletype) {
            case TIME:
                fill_time_col(tab, writemode, times, size);
                int res = compare_time_col(tab, times, size);
                break;
            case UVW:
                // fill_uvw_col(tab, writemode, uvws);
                break;
            case DATA:
            case ALL:
            default:
                printf("todo: tabletype");
                return 1;
        }
        return 0;
    }

    printf("iterations: %d ", iterations);
    printf("\n");

    // gets start time on construction
    Timer timer;
    while (iterations--) {
        switch (tabletype) {
            case TIME:
                fill_time_col(tab, writemode, times, size);
                break;
            case UVW:
                // fill_uvw_col(tab, writemode, uvws);
                break;
            case DATA:
            case ALL:
            default:
                printf("todo: tabletype");
                return 1;
        }

    }

    cout << "user:   " << timer.user () << endl;
    cout << "system: " << timer.system () << endl;
    cout << "real:   " << timer.real () << endl;

    return 0;
}

