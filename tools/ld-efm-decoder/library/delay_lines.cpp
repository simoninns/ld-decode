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

// Decoder ECMA-130 issue 2 delay line examples:
//
// Delay line 1 (32 delays)
// QVector<int> delay_line1 = {0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1}
//
// Delay line M (28 delays)
// QVector<int> delay_lineM = {108,104,100,96,92,88,84,80,76,72,68,64,60,56,52,48,44,40,36,32,28,24,20,16,12,8,4,0}
//
// Delay line 2 (24 delays)
// QVector<int> delay_line2 = {0,0,0,0,2,2,2,2,0,0,0,0,2,2,2,2,0,0,0,0,2,2,2,2}

// Encoder ECMA-130 issue 2 delay line examples:
//
// Delay line 1 (32 delays)
// QVector<int> delay_line1 = {1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0}
//
// Delay line M (28 delays)
// QVector<int> delay_lineM = {0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60, 64, 68, 72, 76, 80, 84, 88, 92, 96, 100, 104, 108}
//
// Delay line 2 (24 delays)
// QVector<int> delay_line2 = {2,2,2,2,0,0,0,0,2,2,2,2,0,0,0,0,2,2,2,2,0,0,0,0}

DelayLines::DelayLines(QVector<int> _delay_lengths) : delay_lengths(_delay_lengths) {
    // Record the largest delay length
    max_delay = *std::max_element(delay_lengths.begin(), delay_lengths.end());
    
    // Initialize delay buffers
    for (int delay : delay_lengths) {
        QQueue<uint8_t> buffer;
        for (int j = 0; j < delay; ++j) {
            buffer.enqueue(0);
        }
        delay_buffers.append(buffer);
    }

    // Initialize push counter
    push_count = 0;
}

QVector<uint8_t> DelayLines::push(QVector<uint8_t> input_data) {
    if (input_data.size() != delay_lengths.size()) {
        qFatal("DelayLines::push(): Input data size of %d does not match the number of delays (%d).", input_data.size(), delay_lengths.size());
    }

    QVector<uint8_t> output_data(delay_lengths.size(), 0);

    for (int i = 0; i < delay_lengths.size(); ++i) {
        delay_buffers[i].enqueue(input_data[i]);
        if (delay_buffers[i].size() > delay_lengths[i]) {
            output_data[i] = delay_buffers[i].dequeue();
        } else {
            output_data[i] = 0; // or some other default value
        }
    }

    push_count++; // Increment push counter

    return output_data;
}

// The delay line isn't ready until the number of pushes equals or exceeds the largest delay length
// This is because when the number of pushes is less, the output data is not generated just from the input data
// but also from the default values in the delay buffer
//
// Return true if the number of pushes equals or exceeds the largest delay length
bool DelayLines::is_ready() {
    return push_count >= max_delay;
}

int32_t DelayLines::get_number_of_delays() {
    return delay_lengths.size();
}

// Flush the delay lines so they are empty
void DelayLines::flush() {
    for (int i = 0; i < delay_lengths.size(); ++i) {
        delay_buffers[i].clear();
    }
    push_count = 0;
}