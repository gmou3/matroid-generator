#include <omp.h>

#include <iostream>
#include <fstream>
#include <queue>
#include <sstream>
#include <iomanip>  // for std::setw and std::setfill
#include <filesystem>
namespace fs = std::filesystem;

#include "matroid.h"

struct Line {
    size_t file_index;
    std::string revlex;
    size_t line_index;

    bool operator>(const Line& other) const {
        // Min-heap: smaller index comes first
        return line_index > other.line_index;
    }
};

// Generate filename based on n, r, and thread number
inline std::stringstream generate_filename(int n, int r, int thread_num) {
    std::stringstream filename;
    filename << "output" << "/"
             << "n" << std::setw(2) << std::setfill('0') << n
             << "r" << std::setw(2) << std::setfill('0') << r
             << "-thread" << std::setw(2) << std::setfill('0') << thread_num;

    return filename;
}

// Open files (one for each thread) and return file names
inline std::vector<std::string> open_files(int n, int r, int threads) {
    if (!fs::exists("output")) {
        if (!fs::create_directory("output")) {
            std::cerr << "Error: Could not create output directory" << std::endl;
            return {};
        }
    }

    std::vector<std::string> filenames;
    for (int i = 0; i < threads; ++i) {
        std::stringstream filename = generate_filename(n, r, i);
        std::ofstream file(filename.str(), std::ios::binary);
        if (!file) {
            std::cerr << "Error: Could not create file " << filename.str() << std::endl;
            return {};
        }
        filenames.push_back(filename.str());
    }

    return filenames;
}

// Output matroid either to file or to stdout
inline void output_matroid(const Matroid& M, const bool& to_file, const std::string& filename, const int& index) {
    if (to_file) {
        std::ofstream file(filename, std::ios::binary | std::ios::app);
        if (file.is_open()) {
            file << M.revlex << " " << index << "\n";  // include index for sorting later
        } else {
            std::cerr << "Error opening file: " << filename << std::endl;
        }
    } else {
#pragma omp critical(io)
        {
            std::cout << M.revlex << std::endl;
        }
    }
}

// Merge the files and sort the matroids to coincide with single-threaded output
inline void mergesort_and_delete(const std::vector<std::string>& filenames) {
    // Open all files for reading
    std::vector<std::ifstream> files(filenames.size());
    for (size_t i = 0; i < filenames.size(); ++i) {
        files[i].open(filenames[i], std::ios::in);
        if (!files[i]) {
            std::cerr << "Error opening file " << filenames[i] << std::endl;
            return;
        }
    }

    // Compute output filename based on the first input file
    std::string first_filename = filenames.front();
    size_t pos = first_filename.find('-');
    std::string output_filename = first_filename.substr(0, pos);  // Get substring up to the first '-'

    // Open output file
    std::ofstream out(output_filename, std::ios::binary);
    if (!out) {
        std::cerr << "Error opening output file for writing" << std::endl;
        return;
    }

    // Priority queue to merge lines (min-heap based on index)
    std::priority_queue<Line, std::vector<Line>, std::greater<Line>> pq;

    // Initialize the priority queue with the first line of each file
    for (size_t i = 0; i < filenames.size(); ++i) {
        std::string line;
        if (std::getline(files[i], line)) {
            std::stringstream ss(line);
            std::string revlex;
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
        out << current.revlex << std::endl;

        // Read the next line from the file that provided the current line
        std::string line;
        if (std::getline(files[current.file_index], line)) {
            std::stringstream ss(line);
            std::string revlex;
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
            std::cerr << "Failed to delete " << filename << std::endl;
        }
    }
}
