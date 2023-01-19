#include <casacore/tables/Tables.h>
#include <casacore/casa/OS/Directory.h>
#include <casacore/casa/OS/Timer.h>
#include <casacore/casa/Arrays/ArrayError.h>

using namespace casacore;

#define N_ITERS 100
#define N_TIMES 12
#define N_ANTS 128
#define N_BLS (N_ANTS * (N_ANTS + 1) / 2)
#define N_CHANS (24 * 32)
#define N_POLS 4

// preprocessor hacks to reuse a list of names in an enum and a char[][]
#define TABLE_TYPES \
    X(TIME), \
    X(UVW), \
    X(DATA), \
    X(COLUMNWISE), \
    X(ROWWISE)

#define WRITE_MODES \
    X(CELL), \
    X(CELLS), \
    X(COLUMN)

// make an enum of table types
#define X(name) name
typedef enum TableType {
    TABLE_TYPES
} TableType;
#define NUM_TABLETYPES (sizeof(tableTypeNames) / sizeof(tableTypeNames[0]))
#define DEFAULT_TABLETYPE COLUMNWISE

typedef enum WriteMode {
    WRITE_MODES
} WriteMode;
#define NUM_WRITEMODES (sizeof(writeModeNames) / sizeof(writeModeNames[0]))
#define DEFAULT_WRITEMODE CELL
#undef X

#define X(name) #name
char const *tableTypeNames[] = {
    TABLE_TYPES
};
char const *writeModeNames[] = {
    WRITE_MODES
};
#undef X

TableType tableTypeFromName(std::string& name) {
    for (auto & c: name) c = toupper(c);
    for (unsigned int i = 0; i < NUM_TABLETYPES; i++) {
        if (name == tableTypeNames[i]) {
            return (TableType) i;
        }
    }
    throw std::runtime_error("unknown table type: " + name);
}

WriteMode writeModeFromName(std::string& name) {
    for (auto & c: name) c = toupper(c);
    for (unsigned int i = 0; i < NUM_WRITEMODES; i++) {
        if (name == writeModeNames[i]) {
            return (WriteMode) i;
        }
    }
    throw std::runtime_error("unknown write mode: " + name);
}

void usage(char const *argv[]) {
    std::cout << "Usage: " << argv[0] << " [-h] [-v|-q] [-V] [-s] [-i <iterations>] [-t <tabletype>] [-w <writemode>]" \
        << " [-T <times>] [-B <baselines>] [-C <chans>] [-P <pols>]\n" \
        << "  -h: print this help message\n" \
        << "  -v: increase verbosity\n" \
        << "  -q: decrease verbosity\n" \
        << "  -V: validate the table values\n" \
        << "  -s: stream junk to the table instead of slicing a pre-allocated array\n"
        << "  -i <iterations>: number of iterations (default: " << N_ITERS << " )\n" \
        << "  -t <tabletype>: table type (default: " << tableTypeNames[DEFAULT_TABLETYPE] << ")\n" \
        << "    options: ";
        for (unsigned int i = 0; i < NUM_TABLETYPES; i++) {
            std::cout << tableTypeNames[i];
            if (i < NUM_TABLETYPES-1) {
                std::cout << ", ";
            }
        }
        std::cout << "\n" \
        << "  -w <writemode>: write mode (default: " << writeModeNames[DEFAULT_WRITEMODE] << ")\n" \
        << "    options: ";
        for (unsigned int i = 0; i < NUM_WRITEMODES; i++) {
            std::cout << writeModeNames[i];
            if (i < NUM_WRITEMODES-1) {
                std::cout << ", ";
            }
        }
        std::cout << "\n" \
        << "  -T <times>: number of times (default: " << N_TIMES << ")\n" \
        << "  -B <baselines>: number of baselines (default: " << N_BLS << ")\n" \
        << "  -C <chans>: number of channels (default: " << N_CHANS << ")\n" \
        << "  -P <pols>: number of polarizations (default: " << N_POLS << ")\n";
}

typedef struct Args {
    int nIters = N_ITERS;
    int nTimes = N_TIMES;
    int nBls = N_BLS;
    int nChs = N_CHANS;
    int nPols = N_POLS;
    int verbosity = 0;
    WriteMode writeMode = DEFAULT_WRITEMODE;
    TableType tableType = DEFAULT_TABLETYPE;
    bool validate = false;
    bool stream = false;
} Args;

// A table containing:
// - a scalar double TIME column
// - an array[3] float UVW column
// - an array[N_CHANS, N_POLS] complex DATA column
Table setup_table(const String& tableName, Args args) {
    if (args.verbosity > 0) {
        cout << "setting up table" << endl;
    }
    Directory dir("data");
    if (dir.exists()) {
        dir.remove();
    }
    // from https://casacore.github.io/casacore/group__Tables__module.html#Tables:creation
    // Step1 -- Build the table description.
    TableDesc td("tTableDesc", "1D", TableDesc::Scratch);
    td.comment() = "A table with a single column of double values.";
    ScalarColumnDesc<Double> timeColDesc("TIME");
    ArrayColumnDesc<Float> uvwColDesc("UVW", IPosition(1, 3), ColumnDesc::Direct | ColumnDesc::FixedShape);
    ArrayColumnDesc<Complex> dataColDesc("DATA", IPosition(2, args.nPols, args.nChs), ColumnDesc::FixedShape);
    switch (args.tableType) {
        case TIME:
            td.addColumn (timeColDesc);
            break;
        case UVW:
            td.addColumn (uvwColDesc);
            break;
        case DATA:
            td.addColumn (dataColDesc);
            break;
        default:
            td.addColumn (timeColDesc);
            td.addColumn (uvwColDesc);
            td.addColumn (dataColDesc);
            break;
    }

    SetupNewTable newtab(tableName, td, Table::New);

    Timer timer;
    Table tab(newtab, args.nTimes * args.nBls);
    if (args.verbosity > 0) {
        std::cout << "table setup time: " << endl;
        std::cout << "- user:   " << timer.user () << "s" << endl;
        std::cout << "- system: " << timer.system () << "s" << endl;
        std::cout << "- real:   " << timer.real () << "s" << endl;
    }

    return tab;
}

// Synthesize test data for the table
void synthesize_data(Vector<Double>& times, Array<Float>& uvws, Array<Complex>& data, Args args) {
    if (args.verbosity > 0) {
        cout << "synthesizing data" << endl;
    }
    if (args.stream) {
        switch (args.writeMode) {
            case CELL:
                times.resize(1);
                uvws.resize(IPosition(2, 3, 1));
                data.resize(IPosition(3, args.nPols, args.nChs, 1));
                break;
            case CELLS:
                times.resize(args.nBls);
                uvws.resize(IPosition(2, 3, args.nBls));
                data.resize(IPosition(3, args.nPols, args.nChs, args.nBls));
                break;
            case COLUMN:
                cerr << "warning: streaming a column does not avoid avoid slicing." << endl;
                break;
        }
    }
    indgen(times);
    // indgen(uvws);
    IPosition uvwShape = uvws.shape();
    IPosition dataShape = data.shape();
    if (args.verbosity > 0) {
        cout << "uvw shape: " << uvwShape << endl;
        cout << "data shape: " << dataShape << endl;
    }
    for (int i = 0; i < uvwShape[1]; i++) {
        for (int j = 0; j < uvwShape[0]; j++) {
            uvws(IPosition(2, j, i)) = i + (j+1) * 0.1;
        }
    }
    // indgen(data);
    for (int i = 0; i < dataShape[2]; i++) {
        for (int j = 0; j < dataShape[1]; j++) {
            for (int k = 0; k < dataShape[0]; k++) {
                data(IPosition(3, k, j, i)) = Complex(i, j + (k+1) * 0.1);
            }
        }
    }
    if (args.writeMode == CELL) {
        uvws.removeDegenerate();
        data.removeDegenerate();
    }
}

// fill the time column by slicing times for the given write mode
void fill_time_col(Table& tab, Vector<Double>& times, Args& args) {
    ScalarColumn<Double> timeCol(tab, "TIME");
    int nRows = args.nTimes * args.nBls;
    switch (args.writeMode) {
        case CELL:
            for (int i = 0; i < nRows; i++) {
                timeCol.put(i, times[i]);
            }
            break;
        case CELLS:
            for (int i = 0; i < args.nTimes; i++) {
                Slicer chunker( IPosition(1, i * args.nBls), IPosition(1, args.nBls));
                timeCol.putColumnRange(chunker, times(chunker));
            }
            break;
        case COLUMN:
            timeCol.putColumn(times);
            break;
    }
}

// stream a pre-sliced array into into the time table.
void stream_time_col(Table& tab, Vector<Double> times, Args& args) {
    ScalarColumn<Double> timeCol(tab, "TIME");
    int nRows = args.nTimes * args.nBls;
    switch (args.writeMode) {
        case CELL:
            for (int i = 0; i < nRows; i++) {
                timeCol.put(i, times[0]);
            }
            break;
        case CELLS:
            for (int i = 0; i < args.nTimes; i++) {
                // Slicer chunker( IPosition(1, i * args.nBls), IPosition(1, args.nBls));
                casacore::RefRows rownrs(i * args.nBls, (i + 1) * args.nBls - 1);
                timeCol.putColumnCells(rownrs, times);
            }
            break;
        case COLUMN:
            timeCol.putColumn(times);
            break;
    }
}

void fill_uvw_col(Table& tab, Array<Float>& uvws, Args& args) {
    ArrayColumn<Float> uvwCol(tab, "UVW");
    int nRows = args.nTimes * args.nBls;
    switch (args.writeMode) {
        case CELL:
            for (int i = 0; i < nRows; i++) {
                Array<Float> row = uvws(Slicer(IPosition(2, 0, i), IPosition(2, Slicer::MimicSource, 1)));
                row.removeDegenerate();
                uvwCol.put(i, row);
            }
            break;
        case CELLS:
            for (int i = 0; i < args.nTimes; i++) {
                casacore::RefRows rownrs(i * args.nBls, (i + 1) * args.nBls - 1);
                Slicer chunker( IPosition(2, 0, i * args.nBls), IPosition(2, Slicer::MimicSource, args.nBls));
                uvwCol.putColumnCells(rownrs, uvws(chunker));
            }
            break;
        case COLUMN:
            uvwCol.putColumn(uvws);
            break;
    }
}

void stream_uvw_col(Table& tab, Array<Float>& uvws, Args& args) {
    ArrayColumn<Float> uvwCol(tab, "UVW");
    int nRows = args.nTimes * args.nBls;
    switch (args.writeMode) {
        case CELL:
            for (int i = 0; i < nRows; i++) {
                uvwCol.put(i, uvws);
            }
            break;
        case CELLS:
            for (int i = 0; i < args.nTimes; i++) {
                casacore::RefRows rownrs(i * args.nBls, (i + 1) * args.nBls - 1);
                uvwCol.putColumnCells(rownrs, uvws);
            }
            break;
        case COLUMN:
            uvwCol.putColumn(uvws);
            break;
    }
}

void fill_data_col(Table& tab, Array<Complex>& data, Args& args) {
    ArrayColumn<Complex> dataCol(tab, "DATA");
    int nRows = args.nTimes * args.nBls;
    switch (args.writeMode) {
        case CELL:
            for (int i = 0; i < nRows; i++) {
                Array<Complex> row = data(Slicer(IPosition(3, 0, 0, i), IPosition(3, Slicer::MimicSource, Slicer::MimicSource, 1)));
                row.removeDegenerate();
                dataCol.put(i, row);
            }
            break;
        case CELLS:
            for (int i = 0; i < args.nTimes; i++) {
                casacore::RefRows rownrs(i * args.nBls, (i + 1) * args.nBls - 1);
                Slicer chunker( IPosition(3, 0, 0, i * args.nBls), IPosition(3, Slicer::MimicSource, Slicer::MimicSource, args.nBls));
                dataCol.putColumnCells(rownrs, data(chunker));
            }
            break;
        case COLUMN:
            dataCol.putColumn(data);
            break;
    }
}

void stream_data_col(Table& tab, Array<Complex>& data, Args& args) {
    ArrayColumn<Complex> dataCol(tab, "DATA");
    int nRows = args.nTimes * args.nBls;
    switch (args.writeMode) {
        case CELL:
            for (int i = 0; i < nRows; i++) {
                dataCol.put(i, data);
            }
            break;
        case CELLS:
            for (int i = 0; i < args.nTimes; i++) {
                casacore::RefRows rownrs(i * args.nBls, (i + 1) * args.nBls - 1);
                dataCol.putColumnCells(rownrs, data);
            }
            break;
        case COLUMN:
            dataCol.putColumn(data);
            break;
    }
}

void fill_rowwise(Table& tab, Vector<Double>& times, Array<Float>& uvws, Array<Complex>& data, Args& args) {
    ScalarColumn<Double> timeCol(tab, "TIME");
    ArrayColumn<Float> uvwCol(tab, "UVW");
    ArrayColumn<Complex> dataCol(tab, "DATA");
    int nRows = args.nTimes * args.nBls;
    switch (args.writeMode) {
        case CELL:
            for (int i = 0; i < nRows; i++) {
                timeCol.put(i, times[i]);
                Array<Float> uvw = uvws(Slicer(IPosition(2, 0, i), IPosition(2, Slicer::MimicSource, 1)));
                uvw.removeDegenerate();
                uvwCol.put(i, uvw);
                Array<Complex> row = data(Slicer(IPosition(3, 0, 0, i), IPosition(3, Slicer::MimicSource, Slicer::MimicSource, 1)));
                row.removeDegenerate();
                dataCol.put(i, row);
            }
            break;
        case CELLS:
            for (int i = 0; i < args.nTimes; i++) {
                Slicer chunker( IPosition(1, i * args.nBls), IPosition(1, args.nBls));
                timeCol.putColumnRange(chunker, times(chunker));
                chunker = Slicer( IPosition(2, 0, i * args.nBls), IPosition(2, Slicer::MimicSource, args.nBls));
                Array<Float> uvwChunk = uvws(chunker);
                casacore::RefRows rownrs(i * args.nBls, (i + 1) * args.nBls - 1);
                uvwCol.putColumnCells(rownrs, uvwChunk);
                chunker = Slicer( IPosition(3, 0, 0, i * args.nBls), IPosition(3, Slicer::MimicSource, Slicer::MimicSource, args.nBls));
                Array<Complex> dataChunk = data(chunker);
                dataCol.putColumnCells(rownrs, dataChunk);
            }
            break;
        case COLUMN:
            throw std::runtime_error("can't write rowwise in COLUMN mode");
    }
}

void stream_rowwise(Table& tab, Vector<Double>& times, Array<Float>& uvws, Array<Complex>& data, Args& args) {
    ScalarColumn<Double> timeCol(tab, "TIME");
    ArrayColumn<Float> uvwCol(tab, "UVW");
    ArrayColumn<Complex> dataCol(tab, "DATA");
    int nRows = args.nTimes * args.nBls;
    switch (args.writeMode) {
        case CELL:
            for (int i = 0; i < nRows; i++) {
                timeCol.put(i, times[0]);
                uvwCol.put(i, uvws);
                dataCol.put(i, data);
            }
            break;
        case CELLS:
            for (int i = 0; i < args.nTimes; i++) {
                casacore::RefRows rownrs(i * args.nBls, (i + 1) * args.nBls - 1);
                // Slicer chunker( IPosition(1, i * args.nBls), IPosition(1, args.nBls));
                timeCol.putColumnCells(rownrs, times);
                uvwCol.putColumnCells(rownrs, uvws);
                dataCol.putColumnCells(rownrs, data);
            }
            break;
        case COLUMN:
            throw std::runtime_error("can't write rowwise in COLUMN mode");
    }
}

void compare_time_col(Table& tab, Vector<Double>& times, Args& args) {
    ScalarColumn<Double> timeCol(tab, "TIME");
    for (int i = 0; i < (args.nTimes * args.nBls); i++) {
        if (timeCol(i) != times[i]) {
            std::ostringstream errStream;
            errStream << "time mismatch in " << tab.tableName() << " at row=" << i << ": " << timeCol(i) << " != " << times[i];
            throw std::runtime_error(errStream.str());
        }
    }
}

void compare_uvw_col(Table& tab, Array<Float>& uvws, Args& args) {
    IPosition shape = uvws.shape();
    ArrayColumn<Float> uvwCol(tab, "UVW");
    Array<Float> uvwValues = uvwCol.getColumn();
    if (shape != uvwValues.shape()) {
        std::ostringstream errStream;
        errStream << "uvw shape mismatch in " << tab.tableName();
        throw ArrayShapeError(shape, uvwValues.shape(), errStream.str().c_str());
    }
    for( int i = 0; (rownr_t)i < (uvwCol.nrow()); i++ ) {
        Array<Float> actual = uvwCol(i);
        if (args.verbosity > 0) {
            std::cout << "actual: " << actual << endl;
        }
        // Array<Float> expected = uvwIter.array();
        Array<Float> expected = uvws(Slicer(IPosition(2, 0, i), IPosition(2, Slicer::MimicSource, 1)));
        expected.removeDegenerate();
        // check the sizes are the same
        if (actual.shape() != expected.shape()) {
            std::ostringstream errStream;
            errStream << "uvw shape mismatch in " << tab.tableName() << " at row=" << i;
            throw ArrayShapeError(actual.shape(), expected.shape(), errStream.str().c_str());
        }
        // check the values are the same
        for (int j = 0; j < shape[0]; j++) {
            IPosition index(1, j);
            if (actual(index) !=  expected(index)) {
                std::ostringstream errStream;
                errStream << "uvw value mismatch in " << tab.tableName() << " at row=" << i << ", [" << j << "]: " \
                    << actual(index) << " != " << expected(index) << " (delta=" << fabs(actual(index)-expected(index)) << ")";
                throw std::runtime_error(errStream.str());
            } else if (args.verbosity > 0) {
                std::cout << "uvw value match in " << tab.tableName() << " at row=" << i << ", [" << j << "]: " \
                    << actual(index) << " == " << expected(index) << endl;
            }
        }
    }
}

// check the values in the data col match the data array
void compare_data_col(Table& tab, Array<Complex>& data, Args& args) {
    IPosition shape = data.shape();
    ArrayColumn<Complex> dataCol(tab, "DATA");
    Array<Complex> dataValues = dataCol.getColumn();
    if (shape != dataValues.shape()) {
        std::ostringstream errStream;
        errStream << "data shape mismatch in " << tab.tableName();
        throw ArrayShapeError(shape, dataValues.shape(), errStream.str().c_str());
    }
    for( int i = 0; (rownr_t)i < (dataCol.nrow()); i++ ) {
        Array<Complex> actual = dataCol(i);
        if (args.verbosity > 0) {
            std::cout << "actual: " << actual << endl;
        }
        // Array<Complex> expected = uvwIter.array();
        Array<Complex> expected = data(Slicer(IPosition(3, 0, 0, i), IPosition(3, Slicer::MimicSource, Slicer::MimicSource, 1)));
        expected.removeDegenerate();
        // check the sizes are the same
        if (actual.shape() != expected.shape()) {
            std::ostringstream errStream;
            errStream << "data shape mismatch in " << tab.tableName() << " at row=" << i;
            throw ArrayShapeError(actual.shape(), expected.shape(), errStream.str().c_str());
        }
        // check the values are the same
        for (int j = 0; j < shape[1]; j++) {
            for (int k = 0; k < shape[0]; k++) {
                IPosition index(2, k, j);
                if (actual(index) !=  expected(index)) {
                    std::ostringstream errStream;
                    errStream << "data value mismatch in " << tab.tableName() << " at row=" << i << ", [" << j << ", " << k << "]: " \
                        << actual(index) << " != " << expected(index) << " (delta=" << fabs(actual(index)-expected(index)) << ")";
                    throw std::runtime_error(errStream.str());
                } else if (args.verbosity > 0) {
                    std::cout << "data value match in " << tab.tableName() << " at row=" << i << ", [" << j << ", " << k << "]: " \
                        << actual(index) << " == " << expected(index) << endl;
                }
            }
        }
    }
}

int main(int argc, char const *argv[])
{
    // default arg values
    Args args;

    // parse argv
    int argi = 0;
    while (++argi < argc) {
        // printf("arg %d: %s\n", argi, argv[argi]);
        if(argv[argi][0] == '-') {
            switch(argv[argi][1]) {
                case 'h':
                    usage(argv);
                    return 0;
                case 'v':
                    args.verbosity++;
                    break;
                case 'q':
                    args.verbosity--;
                    break;
                case 's':
                    args.stream = true;
                    break;
                case 'V':
                    args.validate = true;
                    break;
                case 'i':
                    if (++argi < argc) {
                        args.nIters = atoi(argv[argi]);
                    } else {
                        usage(argv);
                        throw std::runtime_error("missing iterations argument");
                    }
                    break;
                case 't':
                    if (++argi < argc) {
                        std::string tableTypeName(argv[argi]);
                        args.tableType = tableTypeFromName(tableTypeName);
                    } else {
                        usage(argv);
                        throw std::runtime_error("missing tabletype argument");
                    }
                    break;
                case 'w':
                    if (++argi < argc) {
                        std::string writeModeName(argv[argi]);
                        args.writeMode = writeModeFromName(writeModeName);
                    } else {
                        usage(argv);
                        throw std::runtime_error("missing writemode argument");
                    }
                    break;
                case 'T':
                    if (++argi < argc) {
                        args.nTimes = atoi(argv[argi]);
                    } else {
                        usage(argv);
                        throw std::runtime_error("missing times argument");
                    }
                    break;
                case 'B':
                    if (++argi < argc) {
                        args.nBls = atoi(argv[argi]);
                    } else {
                        usage(argv);
                        throw std::runtime_error("missing baselines argument");
                    }
                    break;
                case 'C':
                    if (++argi < argc) {
                        args.nChs = atoi(argv[argi]);
                    } else {
                        usage(argv);
                        throw std::runtime_error("missing chans argument");
                    }
                    break;
                case 'P':
                    if (++argi < argc) {
                        args.nPols = atoi(argv[argi]);
                    } else {
                        usage(argv);
                        throw std::runtime_error("missing pols argument");
                    }
                    break;
                default:
                    usage(argv);
                    std::ostringstream errStream;
                    errStream << "unknown option: " << argv[argi];
                    throw std::runtime_error(errStream.str());
            }
        }
    }

    if (args.stream && args.validate) {
        throw std::runtime_error("stream will fill table with junk, and does not validate");
    }

    if (args.verbosity >= 0) {
        cout << "# nTimes=" << args.nTimes << ", nBls=" << args.nBls << ", nChs=" << args.nChs << ", nPols=" << args.nPols \
            << ", tableType=" << tableTypeNames[args.tableType] << ", writeMode=" << writeModeNames[args.writeMode] \
            << ", iterations=" << args.nIters;
        if (args.stream) {
            cout << ", streaming";
        }
        cout << endl;
        flush(cout);
    }

    Vector<Double> times(args.nTimes * args.nBls);
    Array<Float> uvws(IPosition(2, 3, args.nTimes * args.nBls));
    Array<Complex> data(IPosition(3, args.nPols, args.nChs, args.nTimes * args.nBls));

    synthesize_data(times, uvws, data, args);

    Table tab = setup_table("table.data", args);

    if (args.validate) {
        switch (args.tableType) {
            case TIME:
                fill_time_col(tab, times, args);
                compare_time_col(tab, times, args);
                break;
            case UVW:
                fill_uvw_col(tab, uvws, args);
                compare_uvw_col(tab, uvws, args);
                break;
            case DATA:
                fill_data_col(tab, data, args);
                compare_data_col(tab, data, args);
                break;
            case COLUMNWISE:
                fill_time_col(tab, times, args);
                fill_uvw_col(tab, uvws, args);
                fill_data_col(tab, data, args);
                compare_time_col(tab, times, args);
                compare_uvw_col(tab, uvws, args);
                compare_data_col(tab, data, args);
                break;
            case ROWWISE:
                fill_rowwise(tab, times, uvws, data, args);
                compare_time_col(tab, times, args);
                compare_uvw_col(tab, uvws, args);
                compare_data_col(tab, data, args);
                break;
        }
        printf("PASS\n");
        return 0;
    }

    // gets start time on construction
    Timer timer;
    int i = 0;
    while (i++ < args.nIters) {
        if (args.verbosity >= 0) {
            cerr << "iteration " << i << " of " << args.nIters << "\r";
        }
        switch (args.tableType) {
            case TIME:
                if (args.stream) stream_time_col(tab, times, args); else fill_time_col(tab, times, args);
                break;
            case UVW:
                if (args.stream) stream_uvw_col(tab, uvws, args); else fill_uvw_col(tab, uvws, args);
                break;
            case DATA:
                if (args.stream) stream_data_col(tab, data, args); else fill_data_col(tab, data, args);
                break;
            case COLUMNWISE:
                if (args.stream) {
                    stream_time_col(tab, times, args);
                    stream_uvw_col(tab, uvws, args);
                    stream_data_col(tab, data, args);
                } else {
                    fill_time_col(tab, times, args);
                    fill_uvw_col(tab, uvws, args);
                    fill_data_col(tab, data, args);
                }
                break;
            case ROWWISE:
                if (args.stream) stream_rowwise(tab, times, uvws, data, args); else fill_rowwise(tab, times, uvws, data, args);
                break;
        }
    }
    if (args.nIters > 0) {
        cerr << "                          \r";
        std::cout << "user:   " << timer.user () << "s" << endl;
        std::cout << "system: " << timer.system () << "s" << endl;
        std::cout << "real:   " << timer.real () << "s" << endl;
    }

    return 0;
}

