#include <omp.h>

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <queue>
#include <sstream>

#include "matroid.h"

using namespace std;
namespace fs = filesystem;

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
inline stringstream generate_filename(int n, int r, int thread_num) {
    stringstream filename;
    filename << "output" << "/"
             << "n" << setw(2) << setfill('0') << n << "r" << setw(2)
             << setfill('0') << r << "-thread" << setw(2) << setfill('0')
             << thread_num;

    return filename;
}

// Open files (one for each thread) and return file names
inline vector<string> open_files(int n, int r, int threads) {
    if (!fs::exists("output")) {
        if (!fs::create_directory("output")) {
            cerr << "Error: Could not create output directory" << endl;
            return {};
        }
    }

    vector<string> filenames;
    for (int i = 0; i < threads; ++i) {
        stringstream filename = generate_filename(n, r, i);
        ofstream file(filename.str(), ios::binary);
        if (!file) {
            cerr << "Error: Could not create file " << filename.str() << endl;
            return {};
        }
        filenames.push_back(filename.str());
    }

    return filenames;
}

// Output matroid either to file or to stdout
inline void output_matroid(const Matroid& M, const bool& to_file,
                           const string& filename, const int& index) {
    if (to_file) {
        ofstream file(filename, ios::binary | ios::app);
        if (file.is_open()) {
            file << M.revlex << " " << index
                 << "\n";  // include index for sorting later
        } else {
            cerr << "Error opening file: " << filename << endl;
        }
    } else {
#pragma omp critical(io)
        {
            cout << M.revlex << endl;
        }
    }
}

// Merge the files and sort the matroids to coincide with single-threaded output
inline void mergesort_and_delete(const vector<string>& filenames) {
    // Open all files for reading
    vector<ifstream> files(filenames.size());
    for (size_t i = 0; i < filenames.size(); ++i) {
        files[i].open(filenames[i], ios::in);
        if (!files[i]) {
            cerr << "Error opening file " << filenames[i] << endl;
            return;
        }
    }

    // Compute output filename based on the first input file
    string first_filename = filenames.front();
    size_t pos = first_filename.find('-');
    string output_filename =
        first_filename.substr(0, pos);  // Get substring up to the first '-'

    // Open output file
    ofstream out(output_filename, ios::binary);
    if (!out) {
        cerr << "Error opening output file for writing" << endl;
        return;
    }

    // Priority queue to merge lines (min-heap based on index)
    priority_queue<Line, vector<Line>, greater<Line>> pq;

    // Initialize the priority queue with the first line of each file
    for (size_t i = 0; i < filenames.size(); ++i) {
        string line;
        if (getline(files[i], line)) {
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
        out << current.revlex << endl;

        // Read the next line from the file that provided the current line
        string line;
        if (getline(files[current.file_index], line)) {
            stringstream ss(line);
            string revlex;
            size_t index;
            ss >> revlex >> index;  // Split the line into revlex and index
            pq.push({current.file_index, revlex, index});
        }
    }

    // Close files
    for (auto& file : files) {
        file.close();
    }

    out.close();

    // Delete input files
    for (const auto& filename : filenames) {
        if (!fs::remove(filename)) {
            cerr << "Failed to delete " << filename << endl;
        }
    }
}
