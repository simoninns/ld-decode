/************************************************************************

    encoders.cpp

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

#include <QVector>
#include <QQueue>
#include <QMap>
#include <QtGlobal>
#include <QDebug>

#include "encoders.h"

// Data24ToF1Frame class implementation
Data24ToF1Frame::Data24ToF1Frame() {}

void Data24ToF1Frame::push_frame(QVector<uint8_t> data) {
    if (data.size() != 24) {
        qFatal("Data24ToF1Frame::push_frame(): Data must be a QVector of 24 integers in the range 0-255.");
    }
    input_buffer.enqueue(data);
    process_queue();
}

F1Frame Data24ToF1Frame::pop_frame() {
    if (!is_ready()) {
        qFatal("Data24ToF1Frame::pop_frame(): No F1 frames are available.");
    }
    return output_buffer.dequeue();
}

void Data24ToF1Frame::process_queue() {
    while (!input_buffer.isEmpty()) {
        QVector<uint8_t> data = input_buffer.dequeue();

        // ECMA-130 issue 2 page 16 - Clause 16
        // All byte pairs are swapped by the F1 Frame encoder
        for (int i = 0; i < data.size(); i += 2) {
            std::swap(data[i], data[i + 1]);
        }

        F1Frame f1_frame;
        f1_frame.set_data(data);
        output_buffer.enqueue(f1_frame);
    }
}

bool Data24ToF1Frame::is_ready() const {
    return !output_buffer.isEmpty();
}

// F1FrameToF2Frame class implementation
F1FrameToF2Frame::F1FrameToF2Frame() 
    : delay_line1({1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0}),
      delay_line2({2,2,2,2,0,0,0,0,2,2,2,2,0,0,0,0,2,2,2,2,0,0,0,0}),
      delay_lineM({0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60, 64, 68, 72, 76, 80, 84, 88, 92, 96, 100, 104, 108})
{}

void F1FrameToF2Frame::push_frame(F1Frame f1_frame) {
    input_buffer.enqueue(f1_frame);
    process_queue();
}

F2Frame F1FrameToF2Frame::pop_frame() {
    if (!is_ready()) {
        qFatal("F1FrameToF2Frame::pop_frame(): No F2 frames are available.");
    }
    return output_buffer.dequeue();
}

// Note: The F2 frames will not be correct until the delay lines are full
// So lead-in is required to prevent loss of the input date.  For now we will
// just discard the data until the delay lines are full.
//
// This will drop 108+2+1 = 111 F2 frames of data - The decoder will also have
// the same issue and will loose another 111 frames of data (so you need at least
// 222 frames of lead-in data to ensure the decoder has enough data to start decoding)
void F1FrameToF2Frame::process_queue() {
    while (!input_buffer.isEmpty()) {
        // Pop the F1 frame and copy the data
        F1Frame f1_frame = input_buffer.dequeue();
        QVector<uint8_t> data = f1_frame.get_data();

        // Process the data
        data = delay_line2.push(data);
        if (data.isEmpty()) continue;

        data = interleave.interleave(data); // 24
        data = circ.c2_encode(data); // 24 + 4 = 28

        data = delay_lineM.push(data); // 28
        if (data.isEmpty()) continue;

        data = circ.c1_encode(data); // 28 + 4 = 32

        data = delay_line1.push(data); // 32     
        if (data.isEmpty()) continue;

        data = inverter.invert_parity(data); // 32

        // Put the resulting data into an F2 frame and push it to the output buffer
        F2Frame f2_frame;
        f2_frame.set_data(data);
        output_buffer.enqueue(f2_frame);
    }
}

bool F1FrameToF2Frame::is_ready() const {
    return !output_buffer.isEmpty();
}

// F2FrameToF3Frame class implementation
F2FrameToF3Frame::F2FrameToF3Frame() {
    symbol_number = 0;
    total_processed_sections = 0;

    frames_per_section = 98;

    // Initialize the subcode object
    subcode.begin_new_track(1, 1); // Track #1, Q mode 1 (CD audio)
}

// Input is 32 bytes of data from the F23 frame payload
void F2FrameToF3Frame::push_frame(F2Frame f2_frame) {
    input_buffer.enqueue(f2_frame);
    process_queue();
}

F3Frame F2FrameToF3Frame::pop_frame() {
    if (!is_ready()) {
        qFatal("F2FrameToF3Frame::pop_frame(): No F3 frames are available.");
    }
    return output_buffer.dequeue();
}

// Process the input queue of F2 frames into F3 frame sections
// Each section consists of 98 F2 frames, the first two of which are sync frames
// The remaining 96 frames are subcode frames.
void F2FrameToF3Frame::process_queue() {
    while (!input_buffer.isEmpty()) {
        F2Frame f2_frame = input_buffer.dequeue();
        F3Frame f3_frame;

        if (symbol_number == 0) {
            f3_frame.set_frame_type_as_sync0();
        } else if (symbol_number == 1) {
            f3_frame.set_frame_type_as_sync1();
        } else {
            // Generate the subcode byte
            f3_frame.set_frame_type_as_subcode(subcode.get_subcode_byte(symbol_number));
        }

        f3_frame.set_data(f2_frame.get_data());
        output_buffer.enqueue(f3_frame);

        symbol_number++;
        if (symbol_number >= frames_per_section) {
            symbol_number = 0;
            total_processed_sections++;

            subcode.next_section();
        }
    }
}

bool F2FrameToF3Frame::is_ready() const {
    return !output_buffer.isEmpty();
}

// F3FrameToChannel class implementation
F3FrameToChannel::F3FrameToChannel(){
    // Output data is a string that represents bits
    // Since we are working with 14-bit EFM symbols, 3 bit merging symbols and
    // a 24 bit frame sync header - the output length of a channel frame is 588 bit (73.5 bytes)
    // so we can't use a QByteArray directly here without introducing unwanted padding
    output_data = "";
    dsv = 0;
    dsv_direction = true;
    total_t_values = 0;
}

void F3FrameToChannel::push_frame(F3Frame f3_frame) {
    input_buffer.enqueue(f3_frame);

    process_queue();
}

QVector<uint8_t> F3FrameToChannel::pop_frame() {
    if (!is_ready()) {
        qFatal("F3FrameToChannel::pop_frame(): No bytes are available.");
    }
    return output_buffer.dequeue();
}

void F3FrameToChannel::process_queue() {
    while (!input_buffer.isEmpty()) {
        // Pop the F3 frame data from the processing queue
        F3Frame f3_frame = input_buffer.dequeue();
        QVector<uint8_t> f3_frame_data = f3_frame.get_data();

        // Process the F3 frame sync header
        QString current_efm = sync_header;
        QString next_efm;
        QString merging_bits;

        // Verify the channel frame output size
        uint32_t channel_frame_size = 0;

        // Pick the subcode value or a sync0/1 symbol based on the frame type    
        if (f3_frame.get_frame_type() == F3Frame::FrameType::SUBCODE) {
            next_efm = convert_8bit_to_efm(f3_frame.get_subcode());
        } else if (f3_frame.get_frame_type() == F3Frame::FrameType::SYNC0) {
            next_efm = convert_8bit_to_efm(256);
        } else {
            next_efm = convert_8bit_to_efm(257);
        }
        
        merging_bits = choose_merging_bits(current_efm, next_efm);
        dsv += add_to_output_data(current_efm + merging_bits);
        channel_frame_size += QString(current_efm + merging_bits).size();

        // Now output the F3 frame subcode data
        current_efm = next_efm;
        next_efm = convert_8bit_to_efm(f3_frame_data[0]);

        merging_bits = choose_merging_bits(current_efm, next_efm);
        dsv += add_to_output_data(current_efm + merging_bits);
        channel_frame_size += QString(current_efm + merging_bits).size();

        // Now output the F3 frame data
        for (uint32_t index = 0; index < f3_frame_data.size(); index++) {
            current_efm = convert_8bit_to_efm(f3_frame_data[index]);
            if (index < f3_frame_data.size()-1) next_efm = convert_8bit_to_efm(f3_frame_data[index+1]);
            else next_efm = sync_header;

            merging_bits = choose_merging_bits(current_efm, next_efm);
            dsv += add_to_output_data(current_efm + merging_bits);
            channel_frame_size += QString(current_efm + merging_bits).size();
        }

        // Check if the channel frame is complete
        if (channel_frame_size != 588) {
            qFatal("F3FrameToChannel::process_queue(): Channel frame size is not 588 bits. It is %d bits", channel_frame_size);
        }

        // Flush the output data to the output buffer
        flush_output_data();
    }
}

bool F3FrameToChannel::is_ready() const {
    return !output_buffer.isEmpty();
}

// Note: There are 257 EFM symbols: 0 to 255 and two additional sync0 and sync1 symbols
QString F3FrameToChannel::convert_8bit_to_efm(uint16_t value) {
    if (value < 258) {
        return efm_lut[value];
    } else {
        qFatal("F3FrameToChannel::convert_8bit_to_efm(): Value must be in the range 0 to 257.");
    }
}

int32_t F3FrameToChannel::add_to_output_data(const QString& data) {

    // Calculate the DSV delta value for the current data
    int32_t dsv_delta = calculate_dsv_delta(data);    

    // Append the actual data to the output data
    output_data += data;

    return dsv_delta;
}

// This function calculates the DSV delta for the input data i.e. the change in DSV
// if the data is used to generate a channel frame.
int32_t F3FrameToChannel::calculate_dsv_delta(const QString data) {
    // The DSV is based on transitions between pits and lands in the EFM data
    // rather than the number of 1s and 0s.

    // If the first value isn't a 1 - count the zeros until the next 1
    // If dsv_direction is true, we increment dsv by the number of zeros
    // If dsv_direction is false, we decrement dsv by the number of zeros
    // Then we flip dsv_direction and repeat until we run out of data

    int32_t dsv_delta = 0;

    for (int i = 0; i < data.size(); ++i) {
        if (data[i] == '1') {
            dsv_direction = !dsv_direction;
        } else {
            int zero_count = 0;
            while (i < data.size() && data[i] == '0') {
                zero_count++;
                i++;
            }
            if (dsv_direction) {
                dsv_delta += zero_count;
            } else {
                dsv_delta -= zero_count;
            }
            dsv_direction = !dsv_direction;
        }
    }

    return dsv_delta;
}

// Return a list of possible merging bit patterns that don't violate the ECMA-130 rules
QStringList F3FrameToChannel::get_possible_merging_bit_patterns(const QString& current_efm, const QString& next_efm) {
    QStringList patterns;
    QStringList possible_patterns = {"000", "001", "010", "100"};
    for (const QString& pattern : possible_patterns) {
        QString combined = current_efm + pattern + next_efm;
        if (!combined.contains("11") && !combined.contains("101")) {
            // Check if there are 10 or more zeros between the 1s
            int zero_count = 0;
            for (int i = 0; i < combined.size(); ++i) {
                if (combined[i] == '0') {
                    zero_count++;
                } else {
                    zero_count = 0;
                }
                if (zero_count > 10) {
                    break;
                }
            }
            if (zero_count < 10) {
                if (current_efm != sync_header && next_efm != sync_header) {
                    if (!combined.contains(sync_header)) {
                        patterns.append(pattern);
                    }
                } else {
                    patterns.append(pattern);
                }
            }
        }
    }

    if (patterns.isEmpty()) {
        qFatal("F3FrameToChannel::get_possible_merging_bit_patterns(): No possible merging bit patterns found.");
    }
    return patterns;
}

// Choose the merging bit pattern that moves the DSV closest to zero
QString F3FrameToChannel::choose_merging_bits(const QString& current_efm, const QString& next_efm) {
    // Get the possible merging bit patterns
    QStringList possible_patterns = get_possible_merging_bit_patterns(current_efm, next_efm);

    // Choose the pattern that moves the DSV closest to zero
    QString best_pattern = possible_patterns[0];
    int best_dsv_delta = std::numeric_limits<int>::max();

    for (const QString& pattern : possible_patterns) {
        int current_dsv_delta = calculate_dsv_delta(current_efm + pattern + next_efm);
        if (std::abs(dsv + current_dsv_delta) < std::abs(dsv + best_dsv_delta)) {
            best_dsv_delta = current_dsv_delta;
            best_pattern = pattern;
        }
    }

    return best_pattern;
}

void F3FrameToChannel::flush_output_data() {
    // Since the base class uses QVector<uint8_t> for the output buffer we have to as well
    QVector<uint8_t> output_bytes;

    //If there are less than 12 bits in the output data, just return
    if (output_data.size() < 12) {
        return;
    }

    // Note: output_data is a string of bits
    while (output_data.size() >= 12) {
        // Is the first bit a 1?
        if (output_data[0] == '1') {
            // Yes, so count the number of 0s until the next 1
            uint32_t zero_count = 0;
            for (uint32_t i = 1; i < output_data.size(); i++) {
                if (output_data[i] == '0') {
                    zero_count++;
                } else {
                    break;
                }
            }

            // The number of zeros is not between 2 and 10 - something is wrong in the input data
            if (zero_count < 2 || zero_count > 10) {
                qInfo() << "F3FrameToChannel::flush_output_data(): Output data:" << output_data;
                qInfo() << "F3FrameToChannel::flush_output_data(): Zero count:" << zero_count;  
                qFatal("F3FrameToChannel::flush_output_data(): Number of zeros between ones is not between 2 and 10.");
            }

            output_data.remove(0, zero_count + 1);

            // Append the T-value to the output bytes (the number of zeros plus 1)
            output_bytes.append(zero_count + 1);
            total_t_values++;
        } else {
            // First bit is zero... input data is invalid!
            qFatal("F3FrameToChannel::flush_output_data(): First bit should not be zero!");
        }
    }

    output_buffer.enqueue(output_bytes);
}

int32_t F3FrameToChannel::get_total_t_values() const {
    return total_t_values;
}

// Define the 24-bit sync header for the F3 frame
const QString F3FrameToChannel::sync_header = "100000000001000000000010";

// Note: The EFM LUT is a list of 257 EFM symbols, each represented as a 14-bit string
// The EFM symbols are indexed from 0 to 257 with the first 256 symbols representing
// the 8-bit data values from 0 to 255 and the last symbols representing the sync header
// sync0 and sync1 respectively.
const QStringList F3FrameToChannel::efm_lut = {
    "01001000100000", "10000100000000", "10010000100000", "10001000100000",
    "01000100000000", "00000100010000", "00010000100000", "00100100000000",
    "01001001000000", "10000001000000", "10010001000000", "10001001000000",
    "01000001000000", "00000001000000", "00010001000000", "00100001000000",
    "10000000100000", "10000010000000", "10010010000000", "00100000100000",
    "01000010000000", "00000010000000", "00010010000000", "00100010000000",
    "01001000010000", "10000000010000", "10010000010000", "10001000010000",
    "01000000010000", "00001000010000", "00010000010000", "00100000010000",
    "00000000100000", "10000100001000", "00001000100000", "00100100100000",
    "01000100001000", "00000100001000", "01000000100000", "00100100001000",
    "01001001001000", "10000001001000", "10010001001000", "10001001001000",
    "01000001001000", "00000001001000", "00010001001000", "00100001001000",
    "00000100000000", "10000010001000", "10010010001000", "10000100010000",
    "01000010001000", "00000010001000", "00010010001000", "00100010001000",
    "01001000001000", "10000000001000", "10010000001000", "10001000001000",
    "01000000001000", "00001000001000", "00010000001000", "00100000001000",
    "01001000100100", "10000100100100", "10010000100100", "10001000100100",
    "01000100100100", "00000000100100", "00010000100100", "00100100100100",
    "01001001000100", "10000001000100", "10010001000100", "10001001000100",
    "01000001000100", "00000001000100", "00010001000100", "00100001000100",
    "10000000100100", "10000010000100", "10010010000100", "00100000100100",
    "01000010000100", "00000010000100", "00010010000100", "00100010000100",
    "01001000000100", "10000000000100", "10010000000100", "10001000000100",
    "01000000000100", "00001000000100", "00010000000100", "00100000000100",
    "01001000100010", "10000100100010", "10010000100010", "10001000100010",
    "01000100100010", "00000000100010", "01000000100100", "00100100100010",
    "01001001000010", "10000001000010", "10010001000010", "10001001000010",
    "01000001000010", "00000001000010", "00010001000010", "00100001000010",
    "10000000100010", "10000010000010", "10010010000010", "00100000100010",
    "01000010000010", "00000010000010", "00010010000010", "00100010000010",
    "01001000000010", "00001001001000", "10010000000010", "10001000000010",
    "01000000000010", "00001000000010", "00010000000010", "00100000000010",
    "01001000100001", "10000100100001", "10010000100001", "10001000100001",
    "01000100100001", "00000000100001", "00010000100001", "00100100100001",
    "01001001000001", "10000001000001", "10010001000001", "10001001000001",
    "01000001000001", "00000001000001", "00010001000001", "00100001000001",
    "10000000100001", "10000010000001", "10010010000001", "00100000100001",
    "01000010000001", "00000010000001", "00010010000001", "00100010000001",
    "01001000000001", "10000010010000", "10010000000001", "10001000000001",
    "01000010010000", "00001000000001", "00010000000001", "00100010010000",
    "00001000100001", "10000100001001", "01000100010000", "00000100100001",
    "01000100001001", "00000100001001", "01000000100001", "00100100001001",
    "01001001001001", "10000001001001", "10010001001001", "10001001001001",
    "01000001001001", "00000001001001", "00010001001001", "00100001001001",
    "00000100100000", "10000010001001", "10010010001001", "00100100010000",
    "01000010001001", "00000010001001", "00010010001001", "00100010001001",
    "01001000001001", "10000000001001", "10010000001001", "10001000001001",
    "01000000001001", "00001000001001", "00010000001001", "00100000001001",
    "01000100100000", "10000100010001", "10010010010000", "00001000100100",
    "01000100010001", "00000100010001", "00010010010000", "00100100010001",
    "00001001000001", "10000100000001", "00001001000100", "00001001000000",
    "01000100000001", "00000100000001", "00000010010000", "00100100000001",
    "00000100100100", "10000010010001", "10010010010001", "10000100100000",
    "01000010010001", "00000010010001", "00010010010001", "00100010010001",
    "01001000010001", "10000000010001", "10010000010001", "10001000010001",
    "01000000010001", "00001000010001", "00010000010001", "00100000010001",
    "01000100000010", "00000100000010", "10000100010010", "00100100000010",
    "01000100010010", "00000100010010", "01000000100010", "00100100010010",
    "10000100000010", "10000100000100", "00001001001001", "00001001000010",
    "01000100000100", "00000100000100", "00010000100010", "00100100000100",
    "00000100100010", "10000010010010", "10010010010010", "00001000100010",
    "01000010010010", "00000010010010", "00010010010010", "00100010010010",
    "01001000010010", "10000000010010", "10010000010010", "10001000010010",
    "01000000010010", "00001000010010", "00010000010010", "00100000010010",
    "00100000000001", "00000000010010"
};
