/************************************************************************

    delay_lines.h

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

#ifndef DELAY_LINES_H
#define DELAY_LINES_H

#include <QVector>
#include <QQueue>
#include <QMap>
#include <cstdint>

class DelayLine2 {
public:
    DelayLine2();
    QVector<uint8_t> process(QVector<uint8_t> input_data);

private:
    QMap<int, QQueue<uint8_t>> delay_buffers;
};

class DelayLine1 {
public:
    DelayLine1();
    QVector<uint8_t> process(QVector<uint8_t> input_data);

private:
    QMap<int, QQueue<uint8_t>> delay_buffers;
};

class DelayLineM {
public:
    DelayLineM();
    QVector<uint8_t> process(QVector<uint8_t> input_data);

private:
    QVector<QQueue<uint8_t>> delay_buffers;
};

#endif // DELAY_LINES_H
