/************************************************************************

    delay_lines.cpp

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

#include <QVector>
#include <QQueue>
#include <QtGlobal>
#include <QDebug>

#include "delay_lines.h"

DelayLine::DelayLine(int size) : size(size), buffer(size) {}

void DelayLine::add(QVector<uint8_t> input) {
    for (int i = 0; i < input.size(); ++i) {
        buffer[(index + i) % size] = input[i];
    }
    index = (index + input.size()) % size;
}

QVector<uint8_t> DelayLine::get(int delay) {
    QVector<uint8_t> output;
    for (int i = 0; i < delay; ++i) {
        output.append(buffer[(index + size - delay + i) % size]);
    }
    return output;
}

DelayLines::DelayLines(int num_lines, int size) {
    for (int i = 0; i < num_lines; ++i) {
        lines.append(DelayLine(size));
    }
}

void DelayLines::add(QVector<uint8_t> input) {
    for (int i = 0; i < lines.size(); ++i) {
        lines[i].add(input);
    }
}

QVector<QVector<uint8_t>> DelayLines::get(int delay) {
    QVector<QVector<uint8_t>> output;
    for (int i = 0; i < lines.size(); ++i) {
        output.append(lines[i].get(delay));
    }
    return output;
}

// DelayLine2 class implementation - This is the "Delay of 2 bytes" delay line shown
// on page 36 of ECMA-130 issue 2
DelayLine2::DelayLine2() {
    int delay = 2;
    for (int i : {4, 5, 6, 7, 12, 13, 14, 15, 20, 21, 22, 23}) {
        QQueue<uint8_t> queue;
        for (int j = 0; j < delay; ++j) {
            queue.enqueue(0);
        }
        delay_buffers[i] = queue;
    }
}

QVector<uint8_t> DelayLine2::process(QVector<uint8_t> input_data) {
    if (input_data.size() != 24) {
        qFatal("DelayLine2::process(): Data must be a QVector of 24 integers in the range 0-255.");
    }

    QVector<uint8_t> output_data(24, 0);

    // Copy the data that doesn't require delay
    for (int i : {0, 1, 2, 3, 8, 9, 10, 11, 16, 17, 18, 19}) {
        output_data[i] = input_data[i];
    }

    // Copy the data that requires delay
    for (int i : {4, 5, 6, 7, 12, 13, 14, 15, 20, 21, 22, 23}) {
        delay_buffers[i].enqueue(input_data[i]);
        output_data[i] = delay_buffers[i].dequeue();
    }

    return output_data;
}

// DelayLine1 class implementation - This is the "Delay of 1 byte" delay line shown
// on page 36 of ECMA-130 issue 2
DelayLine1::DelayLine1() {
    int delay = 1;
    for (int i : {1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31}) {
        QQueue<uint8_t> queue;
        for (int j = 0; j < delay; ++j) {
            queue.enqueue(0);
        }
        delay_buffers[i] = queue;
    }
}

QVector<uint8_t> DelayLine1::process(QVector<uint8_t> input_data) {
    if (input_data.size() != 32) {
        qFatal("DelayLine1::process(): Data must be a QVector of 32 integers in the range 0-255.");
    }

    QVector<uint8_t> output_data(32, 0);

    // Copy the data that doesn't require delay
    for (int i : {0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30}) {
        output_data[i] = input_data[i];
    }

    // Copy the data that requires delay
    for (int i : {1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31}) {
        delay_buffers[i].enqueue(input_data[i]);
        output_data[i] = delay_buffers[i].dequeue();
    }

    return output_data;
}

// DelayLineM class implementation - This is the "Delay lines" shown
// on page 36 of ECMA-130 issue 2 (28 delays ranging from 0 to 108 bytes of delay)
DelayLineM::DelayLineM() {
    QVector<int> delay_lengths = {0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60, 64, 68, 72, 76, 80, 84, 88, 92, 96, 100, 104, 108};
    for (int delay : delay_lengths) {
        QQueue<uint8_t> buffer;
        for (int j = 0; j < delay; ++j) {
            buffer.enqueue(0);
        }
        delay_buffers.append(buffer);
    }
}

QVector<uint8_t> DelayLineM::process(QVector<uint8_t> input_data) {
    if (input_data.size() != 28) {
        qFatal("DelayLineM::process(): Data must be a QVector of 28 integers in the range 0-255.");
    }

    QVector<uint8_t> output_data(28, 0);

    for (int i = 0; i < 28; ++i) {
        delay_buffers[i].enqueue(input_data[i]);
        output_data[i] = delay_buffers[i].dequeue();
    }

    return output_data;
}
