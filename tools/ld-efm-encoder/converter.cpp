/************************************************************************

    converter.cpp

    ld-efm-encoder - EFM data encoder
    Copyright (C) 2025 Simon Inns

    This file is part of ld-decode-tools.

    ld-process-efm is free software: you can redistribute it and/or
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

#include "delay_lines.h"
#include "frame.h"
#include "converter.h"

Converter::Converter(int max_buffer_size) : max_buffer_size(max_buffer_size) {}

void Converter::flush() {
    input_buffer.clear();
    output_buffer.clear();
}

bool Converter::is_ready() const {
    return !output_buffer.isEmpty();
}

// Data24ToF1Frame class implementation
Data24ToF1Frame::Data24ToF1Frame(int max_buffer_size) : Converter(max_buffer_size) {}

void Data24ToF1Frame::push_frame(QVector<uint8_t> data) {
    if (data.size() != 24) {
        qFatal("Data24ToF1Frame::push_frame(): Data must be a QVector of 24 integers in the range 0-255.");
    }
    input_buffer.enqueue(data);
    process_queue();
}

QVector<uint8_t> Data24ToF1Frame::pop_frame() {
    if (!is_ready()) {
        qFatal("Data24ToF1Frame::pop_frame(): No F1 frames are available.");
    }
    return output_buffer.dequeue();
}

void Data24ToF1Frame::process_queue() {
    while (!input_buffer.isEmpty()) {
        QVector<uint8_t> data = input_buffer.dequeue();
        F1Frame f1_frame;
        f1_frame.set_data(data);
        output_buffer.enqueue(f1_frame.get_data());
    }
}

// F1FrameToF2Frame class implementation
F1FrameToF2Frame::F1FrameToF2Frame(int max_buffer_size) : Converter(max_buffer_size), delay_line2(), delay_line1(), delay_lineM() {}

void F1FrameToF2Frame::push_frame(QVector<uint8_t> f1_frame_data) {
    if (f1_frame_data.size() != 24) {
        qFatal("F1FrameToF2Frame::push_frame(): Input must be a QVector of 24 integers.");
    }
    input_buffer.enqueue(f1_frame_data);
    process_queue();
}

QVector<uint8_t> F1FrameToF2Frame::pop_frame() {
    if (!is_ready()) {
        qFatal("F1FrameToF2Frame::pop_frame(): No F2 frames are available.");
    }
    return output_buffer.dequeue();
}

void F1FrameToF2Frame::process_queue() {
    while (!input_buffer.isEmpty()) {
        QVector<uint8_t> data = input_buffer.dequeue();        
        data = delay_line2.process(data);
        data = interleave(data);
        data = encoderC2(data);
        data = delay_lineM.process(data);
        data = encoderC1(data);
        data = delay_line1.process(data);
        data = inverter(data);

        F2Frame f2_frame;
        f2_frame.set_data(data);
        output_buffer.enqueue(f2_frame.get_data());
    }
}

// Perform the interleaving operation on the input data in accordance with
// ECMA-130 issue 2 page 35 ("Interleaving")
QVector<uint8_t> F1FrameToF2Frame::interleave(QVector<uint8_t> data) {
    QVector<uint8_t> interleaved_data(24, 0);
    interleaved_data[0]  = data[0];
    interleaved_data[1]  = data[1];
    interleaved_data[2]  = data[6];
    interleaved_data[3]  = data[7];
    interleaved_data[4]  = data[12];
    interleaved_data[5]  = data[13];
    interleaved_data[6]  = data[18];
    interleaved_data[7]  = data[19];
    interleaved_data[8]  = data[2];
    interleaved_data[9]  = data[3];
    interleaved_data[10] = data[8];
    interleaved_data[11] = data[9];
    interleaved_data[12] = data[14];
    interleaved_data[13] = data[15];
    interleaved_data[14] = data[20];
    interleaved_data[15] = data[21];
    interleaved_data[16] = data[4];
    interleaved_data[17] = data[5];
    interleaved_data[18] = data[10];
    interleaved_data[19] = data[11];
    interleaved_data[20] = data[16];
    interleaved_data[21] = data[17];
    interleaved_data[22] = data[22];
    interleaved_data[23] = data[23];
    return interleaved_data;
}

// Invert the P and Q parity bytes in accordance with
// ECMA-130 issue 2 page 35
QVector<uint8_t> F1FrameToF2Frame::inverter(QVector<uint8_t> data) {
    for (int i = 12; i < 16; ++i) {
        data[i] = ~data[i] & 0xFF;
    }
    for (int i = 28; i < 32; ++i) {
        data[i] = ~data[i] & 0xFF;
    }
    return data;
}

QVector<uint8_t> F1FrameToF2Frame::encoderC2(QVector<uint8_t> data) {
    if (data.size() != 24) {
        qFatal("F1FrameToF2Frame::encoderC2(): Data must be a QVector of 24 integers in the range 0-255.");
    }
    // Implement Reed-Solomon encoding here
    QVector<uint8_t> encoded_data = data; // Placeholder
    while (encoded_data.size() < 28) {
        encoded_data.append(0); // Pad with zeros
    }
    return encoded_data;
}

QVector<uint8_t> F1FrameToF2Frame::encoderC1(QVector<uint8_t> data) {
    if (data.size() != 28) {
        qFatal("F1FrameToF2Frame::encoderC1(): Data must be a QVector of 28 integers in the range 0-255.");
    }
    // Implement Reed-Solomon encoding here
    QVector<uint8_t> encoded_data = data; // Placeholder
    while (encoded_data.size() < 32) {
        encoded_data.append(0); // Pad with zeros
    }
    return encoded_data;
}

// F2FrameToF3Frame class implementation
F2FrameToF3Frame::F2FrameToF3Frame(int max_buffer_size) : Converter(max_buffer_size), section_index(0), frames_per_section(98), total_processed_sections(0) {}

// Input is 32 bytes of data from the F23 frame payload
void F2FrameToF3Frame::push_frame(QVector<uint8_t> f2_frame_data) {
    if (f2_frame_data.size() != 32) {
        qFatal("F2FrameToF3Frame::push_frame(): Input must be a QVector of 32 integers.");
    }
    input_buffer.enqueue(f2_frame_data);
    process_queue();
}

// Ouput is 33 bytes of data for the F3 frame payload (1 subcode byte + 32 data bytes)
QVector<uint8_t> F2FrameToF3Frame::pop_frame() {
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
        QVector<uint8_t> f2_frame_data = input_buffer.dequeue();
        F3Frame f3_frame;

        if (section_index == 0) {
            f3_frame.set_frame_type_as_sync0();
        } else if (section_index == 1) {
            f3_frame.set_frame_type_as_sync1();
        } else {
            // Note: The subcode value is not computed here - this is just for testing
            f3_frame.set_frame_type_as_subcode(0);
        }

        f3_frame.set_data(f2_frame_data);
        output_buffer.enqueue(f3_frame.get_data());

        section_index++;
        if (section_index >= frames_per_section) {
            section_index = 0;
            total_processed_sections++;
        }
    }
}

// F3FrameToChannel class implementation
F3FrameToChannel::F3FrameToChannel(int max_buffer_size) : Converter(max_buffer_size), dsv(0) {
    // Output data is a string that represents bits
    // Since we are working with 14-bit EFM symbols, 3 bit merging symbols and
    // a 24 bit frame sync header - the output length of a channel frame is 588 bit (73.5 bytes)
    // so we can't use a QByteArray directly here without introducing unwanted padding
    output_data = "";
}

void F3FrameToChannel::push_frame(QVector<uint8_t> f3_frame_data) {
    if (f3_frame_data.size() != 33) {
        qFatal("F3FrameToChannel::push_frame(): Input must be a QVector of 33 integers.");
    }
    input_buffer.enqueue(f3_frame_data);
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
        QVector<uint8_t> f3_frame_data = input_buffer.dequeue();

        QString current_efm = sync_header;
        QString next_efm = convert_8bit_to_efm(f3_frame_data[0]);
        QString merging_bits = choose_merging_bits(current_efm, next_efm, dsv);
        dsv = add_to_output_data(current_efm + merging_bits, dsv);
        
        for (uint32_t index = 0; index < 33; index++) {
            current_efm = convert_8bit_to_efm(f3_frame_data[index]);
            if (index < 32) next_efm = convert_8bit_to_efm(f3_frame_data[index + 1]);
            else next_efm = sync_header;

            merging_bits = choose_merging_bits(current_efm, next_efm, dsv);
            dsv = add_to_output_data(current_efm + merging_bits, dsv);
        }

        // Flush the output data to the output buffer
        flush_output_data();
    }
}

// Note: There are 257 EFM symbols: 0 to 255 and two additional sync0 and sync1 symbols
QString F3FrameToChannel::convert_8bit_to_efm(uint8_t value) {
    if (value < 258) {
        return efm_lut[value];
    } else {
        qFatal("F3FrameToChannel::convert_8bit_to_efm(): Value must be in the range 0 to 257.");
    }
}

int F3FrameToChannel::add_to_output_data(const QString& data, int dsv) {
    for (QChar bit : data) {
        if (bit == '1') {
            dsv++;
        } else if (bit == '0') {
            dsv--;
        }
    }

    output_data += data;

    // Double check the merging bits result in valid output data:
    // The rules state that you have to have at lease 2 zeros between each 1 and no more than 10 zeros.
    // So, if we see 11, 101 or 1000000000001 in the output data, something is wrong.
    if (output_data.contains("11") || output_data.contains("101") || output_data.contains("1000000000001")) {
        qFatal("F3FrameToChannel::add_to_output_data(): Invalid value in output_data");
    }

    return dsv;
}

QStringList F3FrameToChannel::get_possible_merging_bit_patterns(const QString& current_efm, const QString& next_efm) {
    QStringList patterns;
    QStringList possible_patterns = {"000", "001", "010", "100"};
    for (const QString& pattern : possible_patterns) {
        QString combined = current_efm + pattern + next_efm;
        if (!combined.contains("11") && !combined.contains("101") && !combined.contains("1000000000001")) {
            if (current_efm != sync_header && next_efm != sync_header) {
                if (!combined.contains(sync_header)) {
                    patterns.append(pattern);
                }
            } else {
                patterns.append(pattern);
            }
        }
    }

    if (patterns.isEmpty()) {
        qFatal("F3FrameToChannel::get_possible_merging_bit_patterns(): No possible merging bit patterns found.");
    }
    return patterns;
}

QString F3FrameToChannel::choose_merging_bits(const QString& current_efm, const QString& next_efm, int dsv) {
    QStringList possible_patterns = get_possible_merging_bit_patterns(current_efm, next_efm);
    if (dsv < 0) {
        return *std::max_element(possible_patterns.begin(), possible_patterns.end(), [](const QString& a, const QString& b) { return a.count('1') < b.count('1'); });
    } else {
        return *std::max_element(possible_patterns.begin(), possible_patterns.end(), [](const QString& a, const QString& b) { return a.count('0') < b.count('0'); });
    }
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
                qFatal("F3FrameToChannel::flush_output_data(): Number of zeros between ones is not between 2 and 10.");
            }

            output_data.remove(0, zero_count + 1);

            // Append the T-value to the output bytes (the number of zeros plus 1)
            output_bytes.append(zero_count + 1);
        } else {
            // First bit is zero... input data is invalid!
            qFatal("F3FrameToChannel::flush_output_data(): First bit should not be zero!");
        }
    }

    output_buffer.enqueue(output_bytes);
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
