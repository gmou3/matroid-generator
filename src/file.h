#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <queue>
#include <sstream>
#include <unistd.h>
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
    string tmp_filename;

    void open_files(size_t seed_matroid_idx) {
        filename = generate_filename(seed_matroid_idx);

        // Create temp file in the same directory using mkstemps
        fs::path final_path(filename);
        string tmpl = final_path.parent_path().string() + "/"
                    + final_path.stem().string() + ".partial.XXXXXX.sz";
        vector<char> buf(tmpl.begin(), tmpl.end());
        buf.push_back('\0');
        int fd = mkstemps(buf.data(), 3);  // 3 = strlen(".sz")
        if (fd < 0) throw runtime_error("mkstemps failed");
        ::close(fd);
        tmp_filename = string(buf.data());

        sz_writer = make_unique<SZWriter>();
        sz_writer->open(tmp_filename);
    }

    void write_colex(const string& line) { sz_writer->write(line); }

    void close_files() {
        sz_writer->close();
        fs::rename(tmp_filename, filename);
    }
};
vector<SeedMatroid> seed_matroid;

// Open colex files (one pair per thread)
void open_files(size_t left_lim, size_t right_lim) {
    if (!fs::exists("output")) fs::create_directory("output");
    seed_matroid.resize(right_lim - left_lim);
    for (size_t i = left_lim; i < right_lim; ++i)
        seed_matroid[i - left_lim].open_files(i);
}

// Output matroid either to SZ file
inline void output_matroid(const Matroid& M, const size_t& index) {
    auto& ts = seed_matroid[index];
    ts.write_colex(M.colex);
}

// Close all files
inline void close_files() {
    for (auto& ts : seed_matroid) ts.close_files();
}
