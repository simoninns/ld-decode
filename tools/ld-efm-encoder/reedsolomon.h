/************************************************************************

    reedsolomon.h

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

#include "ezpwd/rs_base"
#include "ezpwd/rs"

// ezpwd configuration is:
// NAME, TYPE, SYMBOLS, PAYLOAD, POLY, INIT, FCR, AGR

// To find the integer representation of the polynomial P(x)=x^8+x^4+x^3+x^2+1
// treat the coefficients as binary digits, where each coefficient corresponds to a power of x,
// starting from x^0 on the rightmost side. If there is no term for a specific power of x, its coefficient is 0.
//
// Steps:
//     Write the polynomial in terms of its binary representation:
//     P(x)=x^8+x^4+x^3+x^2+1
//
//     The coefficients from x^8 down to x^0 are: 1,0,0,0,1,1,1,0,1.
//
//     Form the binary number from the coefficients:
//     Binary representation: 100011101

//     Convert the binary number to its decimal (integer) equivalent:
//     0b100011101 = 0x11D = 285

class ReedSolomon {

public:
    ReedSolomon();

    QVector<uint8_t> c1_encode(QVector<uint8_t> input);
    QVector<uint8_t> c1_decode(QVector<uint8_t> input);

    QVector<uint8_t> c2_encode(QVector<uint8_t> input);
    QVector<uint8_t> c2_decode(QVector<uint8_t> input);

private:
    // ezpwd C1 ECMA-130 CIRC configuration
    template < size_t SYMBOLS, size_t PAYLOAD > struct C1RS;
    template < size_t PAYLOAD > struct C1RS<255, PAYLOAD>
        : public __RS(C1RS, uint8_t, 255, PAYLOAD, 0x11D, 0, 1, false);

    // ezpwd C2 ECMA-130 CIRC configuration
    template < size_t SYMBOLS, size_t PAYLOAD > struct C2RS;
    template < size_t PAYLOAD > struct C2RS<255, PAYLOAD>
        : public __RS(C2RS, uint8_t, 255, PAYLOAD, 0x11D, 0, 1, false);

    C1RS<255, 255-4> c1rs;
    C2RS<255, 255-4> c2rs;
};