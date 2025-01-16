/************************************************************************

    reedsolomon.cpp

    ld-efm-decoder - EFM data decoder
    Copyright (C) 2025 Simon Inns

    This file is part of ld-decode-tools.

    ld-efm-decoder is free software: you can redistribute it and/or
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
QVector<uint8_t> ReedSolomon::c1_encode(QVector<uint8_t> data) {
    // Convert the QVector to a std::vector for the ezpwd library
    std::vector<uint8_t> tmp_data(data.begin(), data.end());

    // Encode the data
    c1rs.encode(tmp_data);

    // Convert the std::vector back to a QVector
    data = QVector<uint8_t>(tmp_data.begin(), tmp_data.end());

    return data;
}

// Perform a C1 Reed-Solomon decoding operation on the input data
QVector<uint8_t> ReedSolomon::c1_decode(QVector<uint8_t> data) {
    // Convert the QVector to a std::vector for the ezpwd library
    std::vector<uint8_t> tmp_data(data.begin(), data.end());

    // Decode the data
    c1rs.decode(tmp_data);

    // Convert the std::vector back to a QVector
    data = QVector<uint8_t>(tmp_data.begin(), tmp_data.end());

    return data;
}

// Perform a C2 Reed-Solomon encoding operation on the input data
QVector<uint8_t> ReedSolomon::c2_encode(QVector<uint8_t> data) {
    // Convert the QVector to a std::vector for the ezpwd library
    std::vector<uint8_t> tmp_data(data.begin(), data.end());

    c2rs.encode(tmp_data);

    // Convert the std::vector back to a QVector
    data = QVector<uint8_t>(tmp_data.begin(), tmp_data.end());

    return data;
}

// Perform a C2 Reed-Solomon decoding operation on the input data
QVector<uint8_t> ReedSolomon::c2_decode(QVector<uint8_t> data) {
    // Convert the QVector to a std::vector for the ezpwd library
    std::vector<uint8_t> tmp_data(data.begin(), data.end());

    c2rs.decode(tmp_data);

    // Convert the std::vector back to a QVector
    data = QVector<uint8_t>(tmp_data.begin(), tmp_data.end());

    return data;
}