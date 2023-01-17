#include <casacore/tables/Tables.h>
#include <casacore/casa/OS/Directory.h>
#include <casacore/casa/OS/Timer.h>
#include <casacore/casa/Arrays/ArrayError.h>

using namespace casacore;

#define N_TIMES 12
#define N_ANTS 128
#define N_BLS (N_ANTS * (N_ANTS + 1) / 2)
#define N_CHANS (24 * 32)
#define N_POLS 4

// preprocessor hacks to reuse a list of names in an enum and a list of names
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

typedef enum WriteMode {
    WRITE_MODES
} WriteMode;
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
    for (int i = 0; (long unsigned int)i < sizeof(tableTypeNames) / sizeof(tableTypeNames[0]); i++) {
        if (name == tableTypeNames[i]) {
            return (TableType) i;
        }
    }
    throw std::runtime_error("unknown table type: " + name);
}

WriteMode writeModeFromName(std::string& name) {
    for (auto & c: name) c = toupper(c);
    for (int i = 0; (long unsigned int)i < sizeof(writeModeNames) / sizeof(writeModeNames[0]); i++) {
        if (name == writeModeNames[i]) {
            return (WriteMode) i;
        }
    }
    throw std::runtime_error("unknown write mode: " + name);
}


typedef struct Args {
    int nTimes;
    int nBls;
    int nChs;
    int nPols;
    int verbosity;
    WriteMode writeMode;
    TableType tableType;
} Args;

// A table containing:
// - a scalar double TIME column
// - an array[3] float UVW column
// - an array[N_CHANS, N_POLS] complex DATA column
Table setup_table(const String& tableName, Args args) {
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
    td.addColumn (ArrayColumnDesc<Complex> ("DATA", IPosition(2, args.nPols, args.nChs), ColumnDesc::Direct));

    SetupNewTable newtab(tableName, td, Table::New);

    Table tab(newtab, args.nTimes * args.nBls);
    return tab;
}

// Synthesize test data for the table
void synthesize_data(Vector<Double>& times, Array<Float>& uvws, Array<Complex>& data, Args args) {
    indgen(times);
    // indgen(uvws);
    IPosition shape = uvws.shape();
    for (int i = 0; i < shape[1]; i++) {
        for (int j = 0; j < shape[0]; j++) {
            uvws(IPosition(2, j, i)) = i + (j+1) * 0.1;
        }
    }
    // indgen(data);
    IPosition dataShape = data.shape();
    for (int i = 0; i < dataShape[2]; i++) {
        for (int j = 0; j < dataShape[1]; j++) {
            for (int k = 0; k < dataShape[0]; k++) {
                data(IPosition(3, k, j, i)) = Complex(i, j + (k+1) * 0.1);
            }
        }
    }
}

// fill the time column using the given write mode
void fill_time_col(Table& tab, Vector<Double>& times, Args& args) {
    ScalarColumn<Double> timeCol(tab, "TIME");
    IPosition shape = times.shape();
#ifdef DEBUG
    IPosition expectedShape = IPosition(1, args.nTimes * args.nBls);
    if (shape != expectedShape) {
        throw ArrayShapeError(shape, expectedShape, "time shape mismatch");
    }
#endif
    switch (args.writeMode) {
        case CELL:
            for (int i = 0; i < shape[0]; i++) {
#ifdef DEBUG
                if (args.verbosity > 0) {
                    std::cout << "filling time row " << i << " of " << shape.last() << " with " << times[i] << endl;
                }
#endif
                timeCol.put(i, times[i]);
            }
            break;
        case CELLS:
            for (int i = 0; i < args.nTimes; i++) {
                Slicer chunker( IPosition(1, i * args.nBls), IPosition(1, args.nBls));
#ifdef DEBUG
                if (args.verbosity > 0) {
                    std::cout << "filling time chunk " << i << " of " << args.nTimes << " with " << times(chunker) << endl;
                }
#endif
                timeCol.putColumnRange(chunker, times(chunker));
            }
            break;
        case COLUMN:
            timeCol.putColumn(times);
            break;
    }
}

void fill_uvw_col(Table& tab, Array<Float>& uvws, Args& args) {
    ArrayColumn<Float> uvwCol(tab, "UVW");
    IPosition shape = uvws.shape();
#ifdef DEBUG
    IPosition expectedShape = IPosition(2, 3, args.nTimes * args.nBls);
    if (shape != expectedShape) {
        throw ArrayShapeError(shape, expectedShape, "uvw shape mismatch");
    }
#endif
    switch (args.writeMode) {
        case CELL:
            for (int i = 0; i < shape.last(); i++) {
                Array<Float> row = uvws(Slicer(IPosition(2, 0, i), IPosition(2, Slicer::MimicSource, 1)));
                row.removeDegenerate();
#ifdef DEBUG
                if (args.verbosity > 0) {
                    std::cout << "filling uvw row " << i << " of " << shape.last() << " with " << row << endl;
                }
#endif
                uvwCol.put(i, row);
            }
            break;
        case CELLS:
            for (int i = 0; i < args.nTimes; i++) {
                Slicer chunker( IPosition(2, 0, i * args.nBls), IPosition(2, Slicer::MimicSource, args.nBls));
                Array<Float> chunked = uvws(chunker);
#ifdef DEBUG
                if (args.verbosity > 0) {
                    std::cout << "filling uvw chunk " << i << " of " << args.nTimes << " with " << chunked << endl;
                }
#endif
                casacore::RefRows rownrs(i * args.nBls, (i + 1) * args.nBls - 1);
                uvwCol.putColumnCells(rownrs, chunked);
            }
            break;
        case COLUMN:
            uvwCol.putColumn(uvws);
            break;
    }
}

void fill_data_col(Table& tab, Array<Complex>& data, Args& args) {
    ArrayColumn<Complex> dataCol(tab, "DATA");
    IPosition shape = data.shape();
#ifdef DEBUG
    IPosition expectedShape = IPosition(3, args.nPols, args.nChs, args.nTimes * args.nBls);
    if (shape != expectedShape) {
        throw ArrayShapeError(shape, expectedShape, "data shape mismatch");
    }
#endif
    switch (args.writeMode) {
        case CELL:
            for (int i = 0; i < shape.last(); i++) {
                Array<Complex> row = data(Slicer(IPosition(3, 0, 0, i), IPosition(3, Slicer::MimicSource, Slicer::MimicSource, 1)));
                row.removeDegenerate();
#ifdef DEBUG
                if (args.verbosity > 0) {
                    std::cout << "filling data row " << i << " of " << shape.last() << " with " << row << endl;
                }
#endif
                dataCol.put(i, row);
            }
            break;
        case CELLS:
            for (int i = 0; i < args.nTimes; i++) {
                Slicer chunker( IPosition(3, 0, 0, i * args.nBls), IPosition(3, Slicer::MimicSource, Slicer::MimicSource, args.nBls));
                Array<Complex> chunked = data(chunker);
#ifdef DEBUG
                if (args.verbosity > 0) {
                    std::cout << "filling data chunk " << i << " of " << args.nTimes << " with " << chunked << endl;
                }
#endif
                casacore::RefRows rownrs(i * args.nBls, (i + 1) * args.nBls - 1);
                dataCol.putColumnCells(rownrs, chunked);
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
    switch (args.writeMode) {
        case CELL:
            for (int i = 0; i < (args.nTimes * args.nBls); i++) {
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
        // else if (args.verbosity > 0) {
        //     std::cout << "data shape is " << actual.shape();
        // }
        // check the values are the same
        for (int j = 0; j < shape[1]; j++) {
            for (int k = 0; k < shape[0]; k++) {
                IPosition index(2, k, j);
                // if (args.verbosity > 0) {
                //     std::cout << "index " << index << endl;
                //     std::cout << "actual " << actual(index) << " expected " << expected(index) << endl;
                // }
                // if (!complex_eq(actual(index), expected(index))) {
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

void usage(char const *argv[]) {
    std::cout << "Usage: " << argv[0] << "[-h] [-V] [-i <iterations>] [-t <tabletype>] [-w <writemode>]" \
        << " [-T <times>] [-B <baselines>] [-C <chans>] [-P <pols>]\n";
}

int main(int argc, char const *argv[])
{
    // default arg values
    int iterations = 100;
    Args args = {N_TIMES, N_BLS, N_CHANS, N_POLS, 0, CELL, COLUMNWISE};
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
                case 'v':
                    args.verbosity++;
                    break;
                case 'V':
                    validate = true;
                    break;
                case 'i':
                    if (++argi < argc) {
                        iterations = atoi(argv[argi]);
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

    if (args.verbosity >= 0) {
        cout << "nTimes=" << args.nTimes << ", nBls=" << args.nBls << ", nChs=" << args.nChs << ", nPols=" << args.nPols \
            << ", tableType=" << tableTypeNames[args.tableType] << ", writeMode=" << writeModeNames[args.writeMode] \
            << ", iterations=" << iterations << endl;
        flush(cout);
    }

    Vector<Double> times(args.nTimes * args.nBls);
    Array<Float> uvws(IPosition(2, 3, args.nTimes * args.nBls));
    Array<Complex> data(IPosition(3, args.nPols, args.nChs, args.nTimes * args.nBls));

    synthesize_data(times, uvws, data, args);

    Table tab = setup_table("table.data", args);

    if (validate) {
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
    while (iterations--) {
        if (args.verbosity >= 0) {
            cout << "iteration " << i << " of " << iterations << "\r";
            flush(cout);
        }
        switch (args.tableType) {
            case TIME:
                fill_time_col(tab, times, args);
                break;
            case UVW:
                fill_uvw_col(tab, uvws, args);
                break;
            case DATA:
                fill_data_col(tab, data, args);
                break;
            case COLUMNWISE:
                fill_time_col(tab, times, args);
                fill_uvw_col(tab, uvws, args);
                fill_data_col(tab, data, args);
                break;
            case ROWWISE:
                fill_rowwise(tab, times, uvws, data, args);
                break;
        }
    }
    cout << endl;

    std::cout << "user:   " << timer.user () << endl;
    std::cout << "system: " << timer.system () << endl;
    std::cout << "real:   " << timer.real () << endl;

    return 0;
}

