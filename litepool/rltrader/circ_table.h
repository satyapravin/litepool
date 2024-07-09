#pragma once
#include <cstring>
#include <stdexcept>
#include <vector>

namespace Simulator {
class TemporalTable {
public:
    TemporalTable(u_int rows, u_int cols) : currentRow(-1),
                                            NUM_ROWS(rows),
                                            NUM_COLS(cols),
                                            buffer(NUM_ROWS, std::vector<double>(NUM_COLS, 0.0)) {
    }

    void addRow(std::vector<double>& row) {
        if (row.size() != NUM_COLS) throw std::runtime_error("Invalid column size");
        currentRow = (currentRow + 1) % NUM_ROWS;
        std::copy(row.begin(), row.end(), buffer[currentRow].begin());
    }

    int get_lagged_row(u_int lag) {
        if (lag >= NUM_ROWS) throw std::runtime_error("lag greater than available books");
        return (currentRow + NUM_ROWS - lag) % NUM_ROWS;
    }

    const std::vector<double>& get(u_int lag) {
        auto idx = get_lagged_row(lag);
        return buffer[idx];
    }

private:
    u_int currentRow;
    u_int NUM_ROWS;
    u_int NUM_COLS;
    std::vector<std::vector<double>> buffer;
};
}
