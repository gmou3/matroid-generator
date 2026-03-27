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

// Generate filename based on the index of the seed matroid
inline string generate_filename(size_t seed_matroid_idx) {
    stringstream filename;
    filename << "output/n10r05-seedmatroid" << setw(6) << setfill('0')
             << seed_matroid_idx << ".sz";
    return filename.str();
}

struct SeedMatroid {
    unique_ptr<SZWriter> sz_writer;
    string filename;

    // void open_files(size_t n, size_t r, int thread_num) {
    void open_files(size_t seed_matroid_idx) {
        filename = generate_filename(seed_matroid_idx);
        sz_writer = make_unique<SZWriter>();
        sz_writer->open(filename);
    }

    void write_colex(const string& line) { sz_writer->write(line); }

    void close_files() { sz_writer->close(); }
};
vector<SeedMatroid> seed_matroid;

// Open colex and .idx files (one pair per thread)
// void open_files(size_t seed_matroid_idx, int threads) {
void open_files(size_t left_lim, size_t right_lim) {
    if (!fs::exists("output")) fs::create_directory("output");
    seed_matroid.resize(right_lim - left_lim);
    for (size_t i = left_lim; i < right_lim; ++i)
        seed_matroid[i - left_lim].open_files(i);
}

// Output matroid either to file or to stdout
inline void output_matroid(const Matroid& M, const size_t& index) {
    auto& ts = seed_matroid[index];
    ts.write_colex(M.colex);
}

// Close all files
inline void close_files() {
    for (auto& ts : seed_matroid) ts.close_files();
}
