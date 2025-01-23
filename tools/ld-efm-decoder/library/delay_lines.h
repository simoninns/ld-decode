/************************************************************************

    delay_lines.h

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

#ifndef DELAY_LINES_H
#define DELAY_LINES_H

#include <QVector>
#include <QQueue>
#include <QMap>
#include <cstdint>

class DelayLines {
public:
    DelayLines(QVector<int> _delay_lengths);
    QVector<uint8_t> push(QVector<uint8_t> input_data);
    bool is_ready();
    int32_t get_number_of_delays();
    void flush();
    QVector<QQueue<uint8_t>> get_delay_buffers(); // Add this line

private:
    QVector<int> delay_lengths;
    QVector<QQueue<uint8_t>> delay_buffers;
    int32_t max_delay;
    int32_t push_count;
};

#endif // DELAY_LINES_H
