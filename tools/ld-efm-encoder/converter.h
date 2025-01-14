/************************************************************************

    converter.h

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

#ifndef CONVERTER_H
#define CONVERTER_H

#include <QVector>
#include <QQueue>
#include <QMap>
#include <cstdint>
#include "delay_lines.h"
#include "frame.h"

class Converter {
public:
    Converter(int max_buffer_size);
    virtual void push_frame(QVector<uint8_t> frame) = 0;
    virtual QVector<uint8_t> pop_frame() = 0;
    virtual void flush();
    virtual bool is_ready() const;

protected:
    virtual void process_queue() = 0;

    QQueue<QVector<uint8_t>> input_buffer;
    QQueue<QVector<uint8_t>> output_buffer;
    int max_buffer_size;
};

class Data24ToF1Frame : public Converter {
public:
    Data24ToF1Frame(int max_buffer_size);
    void push_frame(QVector<uint8_t> data) override;
    QVector<uint8_t> pop_frame() override;

protected:
    void process_queue() override;
};

class F1FrameToF2Frame : public Converter {
public:
    F1FrameToF2Frame(int max_buffer_size);
    void push_frame(QVector<uint8_t> f1_frame_data) override;
    QVector<uint8_t> pop_frame() override;

protected:
    void process_queue() override;

private:
    QVector<uint8_t> interleave(QVector<uint8_t> data);
    QVector<uint8_t> inverter(QVector<uint8_t> data);
    QVector<uint8_t> encoderC2(QVector<uint8_t> data);
    QVector<uint8_t> encoderC1(QVector<uint8_t> data);

    DelayLine2 delay_line2;
    DelayLine1 delay_line1;
    DelayLineM delay_lineM;
};

class F2FrameToF3Frame : public Converter {
public:
    F2FrameToF3Frame(int max_buffer_size);
    void push_frame(QVector<uint8_t> f2_frame_data) override;
    QVector<uint8_t> pop_frame() override;

protected:
    void process_queue() override;

private:
    int section_index;
    int frames_per_section;
    int total_processed_sections;
};

class F3FrameToChannel : public Converter {
public:
    F3FrameToChannel(int max_buffer_size);
    void push_frame(QVector<uint8_t> f3_frame_data) override;
    QVector<uint8_t> pop_frame() override;

protected:
    void process_queue() override;

private:
    QString convert_8bit_to_efm(uint8_t value);
    int add_to_output_data(const QString& data, int dsv);
    QStringList get_possible_merging_bit_patterns(const QString& current_efm, const QString& next_efm);
    QString choose_merging_bits(const QString& current_efm, const QString& next_efm, int dsv);
    void flush_output_data();

    QString output_data;
    QVector<uint8_t> output_bytes; // Change from QByteArray to QVector<uint8_t>
    int dsv;

    static const QString sync_header;
    static const QStringList efm_lut;
};

#endif // CONVERTER_H

