/************************************************************************

    encoders.h

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

#ifndef ENCODERS_H
#define ENCODERS_H

#include <QVector>
#include <QQueue>
#include <QMap>

#include "delay_lines.h"
#include "frame.h"
#include "reedsolomon.h"
#include "subcode.h"
#include "interleave.h"
#include "inverter.h"

class Data24ToF1Frame {
public:
    Data24ToF1Frame();
    void push_frame(QVector<uint8_t> data);
    F1Frame pop_frame();
    bool is_ready() const;

private:
    void process_queue();

    QQueue<QVector<uint8_t>> input_buffer;
    QQueue<F1Frame> output_buffer;
};

class F1FrameToF2Frame {
public:
    F1FrameToF2Frame();
    void push_frame(F1Frame f1_frame);
    F2Frame pop_frame();
    bool is_ready() const;

private:
    void process_queue();

    QQueue<F1Frame> input_buffer;
    QQueue<F2Frame> output_buffer;

    ReedSolomon circ;

    DelayLines delay_line1;
    DelayLines delay_line2;
    DelayLines delay_lineM;

    Interleave interleave;
    Inverter inverter;
};

class F2FrameToF3Frame {
public:
    F2FrameToF3Frame();
    void push_frame(F2Frame f2_frame);
    F3Frame pop_frame();
    bool is_ready() const;

private:
    void process_queue();

    QQueue<F2Frame> input_buffer;
    QQueue<F3Frame> output_buffer;

    int32_t symbol_number;
    int32_t frames_per_section;
    int32_t total_processed_sections;

    Subcode subcode;
};

class F3FrameToChannel {
public:
    F3FrameToChannel();
    void push_frame(F3Frame f3_frame);
    QVector<uint8_t> pop_frame();
    bool is_ready() const;
    int32_t get_total_t_values() const;

private:
    void process_queue();

    QQueue<F3Frame> input_buffer;
    QQueue<QVector<uint8_t>> output_buffer;

    void write_frame(QString channel_frame);
    QString convert_8bit_to_efm(uint16_t value);

    QString add_merging_bits(QString channel_frame);
    QStringList get_legal_merging_bit_patterns(const QString& current_efm, const QString& next_efm);
    QStringList order_patterns_by_dsv_delta(const QStringList& merging_patterns, const QString& current_efm, const QString& next_efm);
    int32_t calculate_dsv_delta(const QString data);

    int32_t dsv;
    bool dsv_direction;
    int32_t total_t_values;
    QString previous_channel_frame;

    static const QString sync_header;
    static const QStringList efm_lut;
};

#endif // ENCODERS_H

