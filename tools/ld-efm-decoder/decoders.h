/************************************************************************

    decoders.h

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

#ifndef DECODERS_H
#define DECODERS_H

#include <QVector>
#include <QQueue>
#include <QByteArray>

#include "frame.h"

class TvaluesToChannel {
public:
    TvaluesToChannel();
    void push_frame(QByteArray data);
    QString pop_frame();
    bool is_ready() const;
    uint32_t get_invalid_t_values_count() const { return invalid_t_values_count; }
    uint32_t get_valid_t_values_count() const { return valid_t_values_count; }

private:
    void process_queue();

    QQueue<QByteArray> input_buffer;
    QQueue<QString> output_buffer;
    uint32_t invalid_t_values_count;
    uint32_t valid_t_values_count;
};

class ChannelToF3Frame {
public:
    ChannelToF3Frame();
    void push_frame(QString data);
    F3Frame pop_frame();
    bool is_ready() const;
    uint32_t get_invalid_channel_frames_count() const { return invalid_channel_frames_count; }
    uint32_t get_valid_channel_frames_count() const { return valid_channel_frames_count; }

private:
    void process_queue();
    uint16_t convert_efm_to_8bit(QString efm);

    QQueue<QString> input_buffer;
    QQueue<F3Frame> output_buffer;

    QString internal_buffer;

    uint32_t invalid_channel_frames_count;
    uint32_t valid_channel_frames_count;

    static const QString sync_header;
    static const QStringList efm_lut;
};

class F3FrameToF2Frame {
public:
    F3FrameToF2Frame();
    void push_frame(F3Frame data);
    F2Frame pop_frame();
    bool is_ready() const;
    uint32_t get_invalid_f3_frames_count() const { return invalid_f3_frames_count; }
    uint32_t get_valid_f3_frames_count() const { return valid_f3_frames_count; }

private:
    void process_queue();

    QQueue<F3Frame> input_buffer;
    QQueue<F2Frame> output_buffer;

    uint32_t invalid_f3_frames_count;
    uint32_t valid_f3_frames_count;
};

#endif // DECODERS_H