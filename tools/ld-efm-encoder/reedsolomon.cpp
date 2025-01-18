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

// Perform a C2 Reed-Solomon encoding operation on the input data
// This is a (28,24) Reed-Solomon code
//
// Note: Due to the ECMA-130 standard, the parity bytes are inserted into the middle of the data
// so the input data must be 28 bytes long and contain 12 bytes + 4 parity bytes (set to zero) + 12 bytes.
QVector<uint8_t> ReedSolomon::c2_encode(QVector<uint8_t> input_data) {
    // Convert the QVector to a std::vector for the ezpwd library
    std::vector<uint8_t> tmp_data;

    // Copy the input_data and remove the parity bytes in the middle
    // i.e. 12 + 4 + 12 becomes 12 + 12
    tmp_data.insert(tmp_data.end(), input_data.begin(), input_data.begin() + 12);
    tmp_data.insert(tmp_data.end(), input_data.end() - 12, input_data.end());
    
    // Encode to generate parity bytes
    qInfo() << "ReedSolomon::c2_encode - Encoding data input = " << tmp_data;
    c2rs.encode(tmp_data);
    qInfo() << "ReedSolomon::c2_encode - Encoding data output= " << tmp_data;

    // Extract parity symbols
    std::vector<uint8_t> parity(tmp_data.begin() + 24, tmp_data.end());

    // Place the parity bytes back into the middle of the data
    input_data[12] = parity[0]; // Q12n
    input_data[13] = parity[1]; // Q12n+1
    input_data[14] = parity[2]; // Q12n+2
    input_data[15] = parity[3]; // Q12n+3

    return input_data;
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