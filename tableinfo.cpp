#include <casacore/tables/Tables.h>

using namespace casacore;

int main(int argc, char const *argv[]) {
    const char* filename = argv[1];
    Table table(filename);
    std::cout << "Number of rows: " << table.nrow() << endl;
    TableDesc desc = table.tableDesc();
    std::cout << "Number of columns: " << desc.ncolumn() << endl;
    int max_colname_len = 0;
    for (int i = 0; i < desc.ncolumn(); i++) {
        // ColumnDesc colDesc = desc.columnDesc(i);
        // std::cout << desc.columnNames()[i] << colDesc.options() << std::endl;
        max_colname_len = std::max(max_colname_len, (int)desc.columnNames()[i].size());
    }
    std::cout << "F:.isFixedShape(), S:.isScalar(), A:.isArray(), T:.isTable(), D:.options()&Direct, U:.options()&Undefined" << std::endl;
    printf("%-*s F S A T D U\n", max_colname_len, "Name");
    for (int i = 0; i < desc.ncolumn(); i++) {
        ColumnDesc colDesc = desc.columnDesc(i);
        char fixed = ' ';
        if (colDesc.isFixedShape()) fixed = 'F';
        char scalar = ' ';
        if (colDesc.isScalar()) scalar = 'S';
        char array = ' ';
        if (colDesc.isArray()) array = 'A';
        char table = ' ';
        if (colDesc.isTable()) table = 'T';
        char direct = ' ';
        if (colDesc.options() & ColumnDesc::Direct) direct = 'D';
        char undefined = ' ';
        if (colDesc.options() & ColumnDesc::Undefined) undefined = 'U';
        printf("%-*s %1c %1c %1c %1c %1c %1c\n", max_colname_len, desc.columnNames()[i].c_str(),
            fixed, scalar, array, table, direct, undefined);
    }
}