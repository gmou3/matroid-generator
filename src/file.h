#include <omp.h>

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

bool to_file = false;
bool use_compression = false;

// Generate colex or .idx filenames based on n, r, and thread number
inline string generate_filename(size_t n, size_t r, int thread_num,
                                bool idx = false) {
    stringstream filename;
    filename << "output/n" << setw(2) << setfill('0') << n << "r" << setw(2)
             << setfill('0') << r << "-thread" << setw(2) << setfill('0')
             << thread_num;
    if (idx)
        filename << ".idx";
    else if (use_compression)
        filename << ".sz";
    return filename.str();
}

struct ThreadState {
    size_t current_index = SIZE_MAX;  // current index of seed matroid
    size_t cnt = 0;                   // current number of extensions
    ofstream idx_file;                // associated .idx file
    unique_ptr<SZWriter> sz_writer;
    ofstream colex_file;
    string filename;
    string idx_filename;

    void open_files(size_t n, size_t r, int thread_num) {
        filename = generate_filename(n, r, thread_num);
        idx_filename = generate_filename(n, r, thread_num, true);
        idx_file.open(idx_filename);
        if (use_compression) {
            sz_writer = make_unique<SZWriter>();
            sz_writer->open(filename);
        } else {
            colex_file.open(filename, ios::binary);
        }
    }

    void write_colex(const string& line) {
        if (use_compression)
            sz_writer->write(line);
        else
            colex_file << line << "\n";
    }

    void flush_idx() {
        if (cnt > 0) idx_file << current_index << " " << cnt << "\n";
        cnt = 0;
    }

    void close_files() {
        flush_idx();
        idx_file.close();
        if (use_compression)
            sz_writer->close();
        else
            colex_file.close();
    }
};
vector<ThreadState> thread_state;

// Open colex and .idx files (one pair per thread)
void open_files(size_t n, size_t r, int threads) {
    if (!fs::exists("output")) fs::create_directory("output");
    thread_state.resize(threads);
    for (int i = 0; i < threads; ++i) thread_state[i].open_files(n, r, i);
}

// Output matroid either to file or to stdout
inline void output_matroid(const Matroid& M, const size_t& index, int tid) {
    if (to_file) {
        auto& ts = thread_state[tid];
        if (index != ts.current_index) {
            ts.flush_idx();
            ts.current_index = index;
        }
        ts.cnt++;
        ts.write_colex(M.colex);
    } else {
#pragma omp critical(io)
        cout << M.colex << endl;
    }
}

// Merge the colex files, using .idx files for sort order
// The final output file coincides with the output of a single-threaded run
inline void merge_files() {
    size_t nfiles = thread_state.size();

    // Close all thread files
    for (auto& ts : thread_state) ts.close_files();

    // Open colex files for reading
    vector<unique_ptr<SZReader>> sz_readers;
    vector<ifstream> colex_files;
    if (use_compression) {
        sz_readers.resize(nfiles);
        for (size_t i = 0; i < nfiles; ++i) {
            sz_readers[i] = make_unique<SZReader>();
            sz_readers[i]->open(thread_state[i].filename);
        }
    } else {
        colex_files.resize(nfiles);
        for (size_t i = 0; i < nfiles; ++i)
            colex_files[i].open(thread_state[i].filename);
    }

    // Open .idx files for reading
    vector<ifstream> idx_readers(nfiles);
    for (size_t i = 0; i < nfiles; ++i)
        idx_readers[i].open(thread_state[i].idx_filename);

    // Compute output filename from the first thread's filename
    string output_filename = thread_state.front().filename.substr(
        0, thread_state.front().filename.find('-'));
    if (use_compression) output_filename += ".sz";

    unique_ptr<SZWriter> sz_out;
    ofstream out;

    // Open output file
    if (use_compression) {
        sz_out = make_unique<SZWriter>();
        sz_out->open(output_filename);
    } else {
        out.open(output_filename, ios::binary);
    }

    // Seed the priority queue with the first .idx entry from each file
    priority_queue<tuple<size_t, size_t, size_t>,  // index, cnt, file_index
                   vector<tuple<size_t, size_t, size_t>>, greater<>>
        pq;
    for (size_t i = 0; i < nfiles; ++i) {
        size_t index, cnt;
        if (idx_readers[i] >> index >> cnt) pq.push({index, cnt, i});
    }

    // Merge: for each .idx entry in index order, copy cnt colex lines
    while (!pq.empty()) {
        auto [index, cnt, file_index] = pq.top();
        pq.pop();

        for (size_t j = 0; j < cnt; ++j) {
            string line;
            if (use_compression) {
                sz_readers[file_index]->getline(line);
                sz_out->write(line);
            } else {
                getline(colex_files[file_index], line);
                out << line << "\n";
            }
        }

        // Advance to the next .idx entry from the same thread file
        size_t next_index, next_cnt;
        if (idx_readers[file_index] >> next_index >> next_cnt)
            pq.push({next_index, next_cnt, file_index});
    }

    // Close all files
    if (use_compression) {
        for (auto& r : sz_readers) r->close();
        sz_out->close();
    } else {
        for (auto& f : colex_files) f.close();
        out.close();
    }
    for (auto& f : idx_readers) f.close();

    // Delete thread-local files
    for (auto& ts : thread_state) {
        fs::remove(ts.filename);
        fs::remove(ts.idx_filename);
    }
}
