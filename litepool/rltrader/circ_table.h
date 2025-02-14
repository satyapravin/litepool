#pragma once
#include <stdexcept>
#include <vector>
#include <iostream>
#include "fixed_vector.h"

namespace RLTrader {
class TemporalTable {
public:
    explicit TemporalTable(u_int rows) : currentRow(-1), NUM_ROWS(rows), buffer(rows) {
    }

    void addRow(FixedVector<double, 20>& row) {
        if (row.size() != 20) {
	    std::cout << row.size() << std::endl;
            throw std::runtime_error("Invalid column size");
	}
        currentRow = (currentRow + 1) % NUM_ROWS;
        std::ranges::copy(row, buffer[currentRow].begin());
    }

    [[nodiscard]] u_int get_lagged_row(u_int lag) const {
        if (lag >= NUM_ROWS) throw std::runtime_error("lag greater than available books");
        return (currentRow + NUM_ROWS - lag) % NUM_ROWS;
    }

    const FixedVector<double, 20>& get(const u_int lag) const {
        const auto idx = get_lagged_row(lag);
        return buffer[idx];
    }

private:
    u_int currentRow;
    u_int NUM_ROWS;
    std::vector<FixedVector<double, 20>> buffer;
};
}
