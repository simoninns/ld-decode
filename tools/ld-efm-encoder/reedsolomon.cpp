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
QVector<uint8_t> ReedSolomon::c1_encode(QVector<uint8_t> input_data) {
    // Convert the QVector to a std::vector for the ezpwd library
    std::vector<uint8_t> tmp_data(input_data.begin(), input_data.end()-4);

    // Encode the data
    c1rs.encode(tmp_data);

    // Extract parity symbols
    std::vector<uint8_t> parity(tmp_data.begin() + 28, tmp_data.end());

    // Insert the parity bytes back into the original data (bytes 28-31)
    for (int i = 0; i < 4; ++i) {
        input_data[28 + i] = parity[i];
    }

    return input_data;
}

// Perform a C1 Reed-Solomon decoding operation on the input data
QVector<uint8_t> ReedSolomon::c1_decode(QVector<uint8_t> encoded_data) {
    // Convert the QVector to a std::vector for the ezpwd library
    std::vector<uint8_t> tmp_data(encoded_data.begin(), encoded_data.end());

    // Decode the data
    c1rs.decode(tmp_data);

    // Convert the std::vector back to a QVector
    encoded_data = QVector<uint8_t>(tmp_data.begin(), tmp_data.end());

    return encoded_data;
}

QVector<uint8_t> ReedSolomon::c2_encode(QVector<uint8_t> data) {
    // Since we need the 'parity' in the middle of the data (positions 12-15) we
    // have to trick ezpwd into thinking the last 4 bytes of data are parity bytes
    // and ask it to recover the "middle bytes" using the parity bytes at the end.
    //
    // The effect of this is that the last 4 bytes of the input data become the parity
    // and we don't loose any data when the decoder removes the middle 4 bytes.
    //
    // Note: This works because you can recover 4 bytes using RS(28,24) provided you
    // know the positions of the missing bytes in advance.
    //
    // The ECMA-130 doesn't specify how the C2 parity bytes are generated, so this
    // the best method I could come up with.  There don't seem to be any online references
    // that discuss this (all of them just show data in, data out and treat C2 as a black box).

    // Convert the QVector to a std::vector for the ezpwd library
    std::vector<uint8_t> tmp_data;

    // Copy the first 12 bytes of the input data
    tmp_data.insert(tmp_data.end(), data.begin(), data.begin() + 12);

    // Insert 4 zeros for the parity bytes
    tmp_data.insert(tmp_data.end(), 4, 0);

    // Copy the last 12 bytes of the input data
    tmp_data.insert(tmp_data.end(), data.end() - 12, data.end());

    // Mark the parity byte positions as erasures (12-15)
    std::vector<int> erasures = {12, 13, 14, 15};  // 0-based indices of "missing" bytes

    // Decode and correct erasures (i.e. insert the missing parity bytes in the middle)
    c2rs.decode(tmp_data, erasures);

    // Copy the data back to a QVector
    QVector<uint8_t> output_data(tmp_data.begin(), tmp_data.end());
    return output_data;
}

// Perform a C2 Reed-Solomon decoding operation on the input data
QVector<uint8_t> ReedSolomon::c2_decode(QVector<uint8_t> encoded_data) {
    // Convert the QVector to a std::vector for the ezpwd library
    std::vector<uint8_t> tmp_data(encoded_data.begin(), encoded_data.end());

    std::vector<int> position;
    std::vector<int> erasures;
    int fixed = -1;

    // Decode the data
    fixed = c2rs.decode(tmp_data, erasures, &position);

    if (fixed == 0) {
        qDebug() << "ReedSolomon::c2_decode - No errors found";
    } else {
        if (fixed > 0 && fixed <= 3) {
            qDebug() << "ReedSolomon::c2_decode - Fixed" << fixed << "errors";
        } else {
            if (fixed > 3) {
                qDebug() << "ReedSolomon::c2_decode - Too many errors to correct (" << fixed << ") C2 is lost";
                qFatal("ReedSolomon::c2_decode(): Too many errors to correct");
                tmp_data.clear();
            } else {
                qDebug() << "ReedSolomon::c2_decode - Unrecoverable errors found (" << fixed << ") C2 is lost";
                qFatal("ReedSolomon::c2_decode(): Too many errors to correct");
                tmp_data.clear();
            }
        }
    }

    // Convert the std::vector back to a QVector
    encoded_data = QVector<uint8_t>(tmp_data.begin(), tmp_data.end());

    return encoded_data;
}