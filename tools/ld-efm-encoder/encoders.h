/************************************************************************

    encoders.h

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

#ifndef ENCODERS_H
#define ENCODERS_H

#include <QVector>
#include <QQueue>
#include <QMap>
#include <cstdint>

#include "delay_lines.h"
#include "frame.h"
#include "ezpwd/rs_base"
#include "ezpwd/rs"

// ezpwd C1 ECMA-130 CIRC configuration for Reed-Solomon forward error correction
template < size_t SYMBOLS, size_t PAYLOAD > struct C1RS;
template < size_t PAYLOAD > struct C1RS<255, PAYLOAD>
    : public __RS(C1RS, uint8_t, 255, PAYLOAD, 0x11d, 0, 1, false);

// ezpwd C2 ECMA-130 CIRC configuration for Reed-Solomon forward error correction
template < size_t SYMBOLS, size_t PAYLOAD > struct C2RS;
template < size_t PAYLOAD > struct C2RS<255, PAYLOAD>
    : public __RS(C2RS, uint8_t, 255, PAYLOAD, 0x11d, 0, 1, false);

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

    QVector<uint8_t> interleave(QVector<uint8_t> data);
    QVector<uint8_t> inverter(QVector<uint8_t> data);
    QVector<uint8_t> encoderC2(QVector<uint8_t> data);
    QVector<uint8_t> encoderC1(QVector<uint8_t> data);

    DelayLine2 delay_line2;
    DelayLine1 delay_line1;
    DelayLineM delay_lineM;
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

    int section_index;
    int frames_per_section;
    int total_processed_sections;
};

class F3FrameToChannel {
public:
    F3FrameToChannel();
    void push_frame(F3Frame f3_frame);
    QVector<uint8_t> pop_frame();
    bool is_ready() const;

private:
    void process_queue();

    QQueue<F3Frame> input_buffer;
    QQueue<QVector<uint8_t>> output_buffer;

    QString convert_8bit_to_efm(uint16_t value);
    int add_to_output_data(const QString& data, int dsv);
    QStringList get_possible_merging_bit_patterns(const QString& current_efm, const QString& next_efm);
    QString choose_merging_bits(const QString& current_efm, const QString& next_efm, int dsv);
    void flush_output_data();

    QString output_data;
    int dsv;

    static const QString sync_header;
    static const QStringList efm_lut;
};

#endif // ENCODERS_H

