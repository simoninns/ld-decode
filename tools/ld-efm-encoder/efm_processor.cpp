/************************************************************************

    efm_processor.cpp

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
#include <QString>
#include <QFile>

#include "efm_processor.h"
#include "data24.h"
#include "encoders.h"

#include "delay_lines.h"

EfmProcessor::EfmProcessor() {}

bool EfmProcessor::process(QString input_filename, QString output_filename) {
    qDebug() << "EfmProcessor::process(): Encoding EFM data from file:" << input_filename << "to file:" << output_filename;

    Data24 data24(input_filename, is_input_data_wav);
    if (!data24.is_ready()) {
        qDebug() << "EfmProcessor::process(): Failed to load data file:" << input_filename;
        return false;
    }

    // Prepare the output file
    QFile output_file(output_filename);

    if (!output_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qDebug() << "EfmProcessor::process(): Failed to open output file:" << output_filename;
        return false;
    }

    // Audio data
    QVector<uint8_t> data24_frame;
    uint32_t data24_count = 0;

    // Prepare the encoders
    Data24ToF1Frame data24_to_f1;
    F1FrameToF2Frame f1_frame_to_f2;
    F2FrameToF3Frame f2_frame_to_f3;
    F3FrameToChannel f3_to_channel;

    uint32_t f1_frame_count = 0;
    uint32_t f2_frame_count = 0;
    uint32_t f3_frame_count = 0;
    uint32_t channel_byte_count = 0;

    // Process the input audio data 24 bytes at a time
    while (!(data24_frame = data24.read()).isEmpty()) {
        data24_count += 24;

        if (showInput) data24.show_data();

        // Push the data to the first converter
        data24_to_f1.push_frame(data24_frame);

        // Are there any F1 frames ready?
        if (data24_to_f1.is_ready()) {
            // Pop the F1 frame, count it and push it to the next converter
            F1Frame f1_frame = data24_to_f1.pop_frame();
            if (showF1) f1_frame.show_data();
            f1_frame_count++;
            f1_frame_to_f2.push_frame(f1_frame);
        }

        // Are there any F2 frames ready?
        if (f1_frame_to_f2.is_ready()) {
            // Pop the F2 frame, count it and push it to the next converter
            F2Frame f2_frame = f1_frame_to_f2.pop_frame();
            if (showF2) f2_frame.show_data();
            f2_frame_count++;
            f2_frame_to_f3.push_frame(f2_frame);
        }

        // Are there any F3 frames ready?
        if (f2_frame_to_f3.is_ready()) {
            // Pop the F3 frame, count it and push it to the next converter
            F3Frame f3_frame = f2_frame_to_f3.pop_frame();
            if (showF3) f3_frame.show_data();
            f3_frame_count++;
            f3_to_channel.push_frame(f3_frame);
        }

        // Is there any channel data ready?
        if (f3_to_channel.is_ready()) {
            // Pop the channel data, count it and write it to the output file
            QVector<uint8_t> channel_data = f3_to_channel.pop_frame();
            channel_byte_count += channel_data.size();

            // Write the channel data to the output file
            output_file.write(reinterpret_cast<const char*>(channel_data.data()), channel_data.size());
        }
    }

    // Close the output file
    output_file.close();

    qInfo() << "Processed" << data24_count << "data24 frames," << f3_to_channel.get_total_t_values() << "T-values," << f1_frame_count << "F1 frames," <<
                f2_frame_count << "F2 frames," << f3_frame_count << "F3 frames," << channel_byte_count << "channel bytes";
    qInfo() << "Encoding complete";

    return true;
}

void EfmProcessor::set_show_data(bool _showInput, bool _showF1, bool _showF2, bool _showF3) {
    showInput = _showInput;
    showF1 = _showF1;
    showF2 = _showF2;
    showF3 = _showF3;
}

// Set the input data type (true for WAV, false for raw)
void EfmProcessor::set_input_type(bool _wavInput) {
    is_input_data_wav = _wavInput;
}   