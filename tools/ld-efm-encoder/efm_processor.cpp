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
#include "audio.h"
#include "encoders.h"

EfmProcessor::EfmProcessor() {}

bool EfmProcessor::process(QString input_filename, QString output_filename, bool showInput, bool showF1, bool showF2, bool showF3) {
    qDebug() << "EfmProcessor::process(): Encoding EFM data from file: " << input_filename << " to file: " << output_filename;

    AudioToData audio_data(input_filename);
    if (!audio_data.open()) {
        qDebug() << "EfmProcessor::process(): Failed to load audio file: " << input_filename;
        return false;
    }

    // Prepare the output file
    QFile output_file(output_filename);

    if (!output_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qDebug() << "EfmProcessor::process(): Failed to open output file: " << output_filename;
        return false;
    }

    // Audio data
    QVector<uint8_t> audio_data_frame;
    uint32_t audio_data_count = 0;

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
    while (!(audio_data_frame = audio_data.read_24_bytes()).isEmpty()) {
        audio_data_count += 24;

        if (showInput) {
            QString dataString;
            for (int i = 0; i < audio_data_frame.size(); ++i) {
                dataString.append(QString("%1 ").arg(audio_data_frame[i], 2, 16, QChar('0')));
            }
            qDebug().noquote() << "Input data:" << dataString.trimmed();
        }

        // Push the data to the first converter
        data24_to_f1.push_frame(audio_data_frame);

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

    // Close the input and output files
    audio_data.close();
    output_file.close();

    qInfo() << "Processed" << audio_data_count << "bytes audio," << f3_to_channel.get_total_t_values() << "T-values," << f1_frame_count << "F1 frames," <<
                f2_frame_count << "F2 frames," << f3_frame_count << "F3 frames," << channel_byte_count << "channel bytes";
    qInfo() << "Encoding complete";
    return true;
}