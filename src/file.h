#include <lzma.h>
#include <omp.h>

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <queue>
#include <sstream>
#include <vector>

#include "matroid.h"
#include "xz.h"

using namespace std;
namespace fs = filesystem;

bool to_file = false;
bool use_compression = false;
vector<string> filenames;  // one per thread
vector<unique_ptr<XZWriter>> xz_writers;

struct Line {
    size_t file_index;
    string revlex;
    size_t line_index;

    bool operator>(const Line& other) const {
        // Min-heap: smaller index comes first
        return line_index > other.line_index;
    }
};

// Generate filename based on n, r, and thread number
inline stringstream generate_filename(size_t n, size_t r, int thread_num) {
    stringstream filename;
    filename << "output/n" << setw(2) << setfill('0') << n << "r" << setw(2)
             << setfill('0') << r << "-thread" << setw(2) << setfill('0')
             << thread_num;
    if (use_compression) filename << ".xz";
    return filename;
}

// Open files (one for each thread) and return file names
void open_files(size_t n, size_t r, int threads) {
    if (!fs::exists("output")) {
        fs::create_directory("output");
    }

    if (use_compression) {
        xz_writers.resize(threads);
        for (int i = 0; i < threads; ++i) {
            stringstream filename = generate_filename(n, r, i);
            xz_writers[i] = make_unique<XZWriter>();
            xz_writers[i]->open(filename.str());
            filenames.push_back(filename.str());
        }
    } else {
        for (int i = 0; i < threads; ++i) {
            stringstream filename = generate_filename(n, r, i);
            ofstream file(filename.str(), ios::binary);
            filenames.push_back(filename.str());
        }
    }
}

// Output matroid either to file or to stdout
inline void output_matroid(const Matroid& M, const size_t& index) {
    if (to_file) {
        if (use_compression) {
            xz_writers[omp_get_thread_num()]->write(M.revlex + " " +
                                                    to_string(index) + "\n");
        } else {
            ofstream file(filenames[omp_get_thread_num()],
                          ios::binary | ios::app);
            file << M.revlex << " " << index << "\n";
        }
    } else {
#pragma omp critical(io)
        cout << M.revlex << endl;
    }
}

// Merge the files and sort the matroids to coincide with single-threaded output
inline void merge_files(int threads) {
    vector<unique_ptr<XZReader>> xz_readers;
    vector<ifstream> files;

    // Open all files for reading
    if (use_compression) {
        for (auto& writer : xz_writers) {
            if (writer) writer->close();
        }
        xz_readers.resize(filenames.size());
        for (size_t i = 0; i < filenames.size(); ++i) {
            xz_readers[i] = make_unique<XZReader>();
            xz_readers[i]->open(filenames[i]);
        }
    } else {
        files.resize(filenames.size());
        for (size_t i = 0; i < filenames.size(); ++i) {
            files[i].open(filenames[i]);
        }
    }

    // Compute output filename based on the first input file
    string output_filename =
        filenames.front().substr(0, filenames.front().find('-'));
    if (use_compression) output_filename += ".xz";

    unique_ptr<XZWriter> xz_out;
    ofstream out;

    // Open output file
    if (use_compression) {
        xz_out = make_unique<XZWriter>();
        xz_out->open(output_filename, threads);
    } else {
        out.open(output_filename, ios::binary);
    }

    // Priority queue to merge lines (min-heap based on index)
    priority_queue<Line, vector<Line>, greater<Line>> pq;

    // Initialize the priority queue with the first line of each file
    for (size_t i = 0; i < filenames.size(); ++i) {
        string line;
        bool success = use_compression
                           ? xz_readers[i]->getline(line)
                           : static_cast<bool>(getline(files[i], line));

        if (success) {
            stringstream ss(line);
            string revlex;
            size_t index;
            ss >> revlex >> index;  // Split the line into revlex and index
            pq.push({i, revlex, index});
        }
    }

    // Merge the files line by line
    while (!pq.empty()) {
        Line current = pq.top();
        pq.pop();

        // Write the current line to the output file
        if (use_compression) {
            xz_out->write(current.revlex + "\n");
        } else {
            out << current.revlex << endl;
        }

        // Read the next line from the file that provided the current line
        string line;
        bool success =
            use_compression
                ? xz_readers[current.file_index]->getline(line)
                : static_cast<bool>(getline(files[current.file_index], line));

        if (success) {
            stringstream ss(line);
            string revlex;
            size_t index;
            ss >> revlex >> index;  // Split the line into revlex and index
            pq.push({current.file_index, revlex, index});
        }
    }

    // Close files
    if (use_compression) {
        for (auto& reader : xz_readers) reader->close();
        xz_out->close();
    } else {
        for (auto& f : files) f.close();
        out.close();
    }

    // Delete input files
    for (const auto& filename : filenames) {
        fs::remove(filename);
    }
}
