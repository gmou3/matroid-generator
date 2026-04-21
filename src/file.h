#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <queue>
#include <sstream>
#include <vector>

#include "matroid.h"
#include "sz.h"

using namespace std;
namespace fs = filesystem;

struct SZFile {
    unique_ptr<SZWriter> sz_writer;
    string filename;

    void open_file() {
        filename = "output/n10r04.sz";
        sz_writer = make_unique<SZWriter>();
        sz_writer->open(filename);
    }

    void write_colex(const string& line) { sz_writer->write(line); }

    void close_file() { sz_writer->close(); }
};
inline SZFile sz_file;

inline void open_sz_file() {
    if (!fs::exists("output")) fs::create_directory("output");
    sz_file.open_file();
}

// Output matroid to sz_file
inline void output_matroid(const Matroid& M) { sz_file.write_colex(M.colex); }

// Close all sz_files
inline void close_sz_file() { sz_file.close_file(); }
