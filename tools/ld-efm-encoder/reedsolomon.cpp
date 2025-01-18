/************************************************************************

    reedsolomon.cpp

    ld-efm-encoder - EFM data encoder
    Copyright (C) 2025 Simon Inns

    This file is part of ld-decode-tools.

    ld-efm-encoder is free software: you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

************************************************************************/

#include <QDebug>

#include "reedsolomon.h"

#include "ezpwd/rs_base"
#include "ezpwd/rs"

ReedSolomon::ReedSolomon() {}

// Perform a C1 Reed-Solomon encoding operation on the input data
// This is a (32,28) Reed-Solomon code
QVector<uint8_t> ReedSolomon::c1_encode(QVector<uint8_t> data) {
    // Convert the QVector to a std::vector for the ezpwd library
    std::vector<uint8_t> tmp_data(data.begin(), data.end()-4);

    // Encode the data
    c1rs.encode(tmp_data);

    // Extract parity symbols
    std::vector<uint8_t> parity(tmp_data.begin() + 28, tmp_data.end());

    // Insert the parity bytes back into the original data (bytes 28-31)
    for (int i = 0; i < 4; ++i) {
        data[28 + i] = parity[i];
    }

    return data;
}

// Perform a C2 Reed-Solomon encoding operation on the input data
// This is a (28,24) Reed-Solomon code
QVector<uint8_t> ReedSolomon::c2_encode(QVector<uint8_t> data) {
    // Convert the QVector to a std::vector for the ezpwd library
    std::vector<uint8_t> tmp_data(data.begin(), data.end());

    // Encode to generate parity bytes
    c2rs.encode(tmp_data);

    // Extract parity symbols
    std::vector<uint8_t> parity(tmp_data.begin() + 24, tmp_data.end());

    // Insert the parity bytes back into the original data (bytes 12-15)
    for (int i = 0; i < 4; ++i) {
        data[12 + i] = parity[i];
    }

    return data;
}