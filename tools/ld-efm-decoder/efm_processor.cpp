/************************************************************************

    efm_processor.cpp

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

#include <QDebug>
#include <QString>
#include <QFile>

#include "efm_processor.h"
#include "decoders.h"

EfmProcessor::EfmProcessor() {
}

bool EfmProcessor::process(QString input_filename, QString output_filename, bool showOutput, bool showF1, bool showF2, bool showF3) {
    qDebug() << "EfmProcessor::Decode(): Decoding EFM from file: " << input_filename << " to file: " << output_filename;

    // Prepare the input file
    QFile input_file(input_filename);

    if (!input_file.open(QIODevice::ReadOnly)) {
        qDebug() << "EfmProcessor::decode(): Failed to open input file: " << input_filename;
        return false;
    }

    // Prepare the output file
    QFile output_file(output_filename);

    if (!output_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qDebug() << "EfmEncoder::encode(): Failed to open output file: " << output_filename;
        return false;
    }

    // Prepare the decoders
    TvaluesToChannel t_values_to_channel;
    ChannelToF3Frame channel_to_f3;
    F3FrameToF2Frame f3_frame_to_f2;
    F2FrameToF1Frame f2_frame_to_f1;
    F1FrameToData24 f1_frame_to_data24;

    uint32_t data24_count = 0;
    uint32_t f1_frame_count = 0;
    uint32_t f2_frame_count = 0;
    uint32_t f3_frame_count = 0;
    uint32_t channel_byte_count = 0;

    // Process the EFM data in chunks of 100 T-values
    bool end_of_data = false;
    while (!end_of_data) {
        // Read 100 T-values from the input file
        QByteArray t_values = input_file.read(100);
        if (t_values.isEmpty()) {
            end_of_data = true;
            qDebug() << "End of data";
        } else {
            t_values_to_channel.push_frame(t_values);
        }

        // Are there any T-values ready?
        if (t_values_to_channel.is_ready()) {
            QString channel_data = t_values_to_channel.pop_frame();
            channel_to_f3.push_frame(channel_data);
        }

        // Are there any F3 frames ready?
        if (channel_to_f3.is_ready()) {
            F3Frame f3_frame = channel_to_f3.pop_frame();
            if (showF3) f3_frame.show_data();
            f3_frame_to_f2.push_frame(f3_frame);
            f3_frame_count++;
        }

        // Are there any F2 frames ready?
        if (f3_frame_to_f2.is_ready()) {
            F2Frame f2_frame = f3_frame_to_f2.pop_frame();
            if (showF2) f2_frame.show_data();
            f2_frame_to_f1.push_frame(f2_frame);
            f2_frame_count++;
        }

        // Are there any F1 frames ready?
        if (f2_frame_to_f1.is_ready()) {
            F1Frame f1_frame = f2_frame_to_f1.pop_frame();
            if (showF1) f1_frame.show_data();
            f1_frame_to_data24.push_frame(f1_frame);
            f1_frame_count++;
        }

        // Are there any data frames ready?
        if (f1_frame_to_data24.is_ready()) {
            QByteArray data = f1_frame_to_data24.pop_frame();
            output_file.write(data);
            data24_count += 1;

            if (showOutput) {
                QString dataString;
                for (int i = 0; i < data.size(); ++i) {
                    uint8_t byte = static_cast<uint8_t>(data[i]);
                    dataString.append(QString("%1 ").arg(byte, 2, 16, QChar('0')));
                }
                qDebug().noquote() << "Output data:" << dataString.trimmed();
            }
        }
    }

    // Show summary
    qInfo() << "Decoding complete";
    qInfo() << "Processed" << t_values_to_channel.get_valid_t_values_count() << "Valid T-Values and" << t_values_to_channel.get_invalid_t_values_count() << "Invalid T-Values";
    qInfo() << "Processed" << channel_to_f3.get_valid_channel_frames_count() << "Valid Channel Frames and" << channel_to_f3.get_invalid_channel_frames_count() << "Invalid Channel Frames";
    qInfo() << "Processed" << f3_frame_to_f2.get_valid_f3_frames_count() << "Valid F3 Frames and" << f3_frame_to_f2.get_invalid_f3_frames_count() << "Invalid F3 Frames";
    qInfo() << "Processed" << f2_frame_to_f1.get_valid_f2_frames_count() << "Valid F2 Frames and" << f2_frame_to_f1.get_invalid_f2_frames_count() << "Invalid F2 Frames";
    qInfo() << "Processed" << f1_frame_to_data24.get_valid_f1_frames_count() << "Valid F1 Frames and" << f1_frame_to_data24.get_invalid_f1_frames_count() << "Invalid F1 Frames";

    // Get the statistics for the C1 and C2 decoders
    int32_t valid_c1s, fixed_c1s, error_c1s;
    f2_frame_to_f1.get_c1_circ_stats(valid_c1s, fixed_c1s, error_c1s);
    qInfo() << "C1 Decoder: Valid:" << valid_c1s << "- Fixed:" << fixed_c1s << "- Error:" << error_c1s
        << "- Total:" << valid_c1s + fixed_c1s + error_c1s << "- Total errors:" << fixed_c1s + error_c1s;

    int32_t valid_c2s, fixed_c2s, error_c2s;
    f2_frame_to_f1.get_c2_circ_stats(valid_c2s, fixed_c2s, error_c2s);
    qInfo() << "C2 Decoder: Valid:" << valid_c2s << "- Fixed:" << fixed_c2s << "- Error:" << error_c2s
        << "- Total:" << valid_c2s + fixed_c2s + error_c2s << "- Total errors:" << fixed_c2s + error_c2s;
    
    qInfo() << "Processed" << f1_frame_count << "F1 Frames," << f2_frame_count << "F2 Frames," << f3_frame_count << "F3 Frames," << channel_byte_count << "Channel Bytes";

    // Close the input and output files
    input_file.close();
    output_file.close();

    // qInfo() << "Processed" << audio_data_count << "bytes audio," << f1_frame_count << "F1 frames," <<
    //             f2_frame_count << "F2 frames," << f3_frame_count << "F3 frames," << channel_byte_count << "channel bytes";
    qInfo() << "Encoding complete";
    return true;
}