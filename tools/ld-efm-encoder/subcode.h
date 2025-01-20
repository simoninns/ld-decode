/************************************************************************

    subcode.h

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

#ifndef SUBCODE_H
#define SUBCODE_H

class Qmode1 {
public:
    Qmode1();

    enum FrameType {
        AUDIO,
        LEAD_IN,
        LEAD_OUT
    };

    void configure_frame(FrameType frame_type, int32_t _track_number, int32_t _frame_number, int32_t _absolute_frame_number);
    uint8_t get_byte(int32_t byte_number);

private:
    int32_t track_number;
    int32_t frame_number;
    int32_t absolute_frame_number;
    QByteArray channel_data;

    // Parameters common to all QMode 1 frames
    typedef struct qmode1_common_t { 
        uint8_t control_field;
        uint8_t adr_field;
        uint8_t tno_field;
        uint8_t min_field;
        uint8_t sec_field;
        uint8_t frame_field;
        uint8_t zero_field;
    } qmode1_common_t;

    // Parameters specific to the lead-in of a QMode 1 frame
    typedef struct qmode1_lead_in_t { 
        uint8_t point_field;
        uint8_t pmin_field;
        uint8_t psec_field;
        uint8_t pframe_field;
    } qmode1_lead_in_t;

    // Parameters specific to the audio portion of a QMode 1 frame
    typedef struct qmode1_audio_t { 
        uint8_t x_field;
        uint8_t amin_field;
        uint8_t asec_field;
        uint8_t aframe_field;
    } qmode1_audio_t;

    qmode1_audio_t qmode1_audio;
    qmode1_lead_in_t qmode1_lead_in;
    qmode1_common_t qmode1_common;

    void generate_audio();
    void generate_lead_in();
    void generate_common();
    void generate_crc();
    uint16_t crc16(const QByteArray &addr);
    uint32_t int_to_bcd2(uint32_t value);
    uint32_t bcd2_to_int(uint32_t bcd);
};

// Q Channel classes
class Qchannel {
public:
    Qchannel();
    void generate_frame(uint8_t _qmode, int32_t _track_number, int32_t _frame_number, int32_t _absolute_frame_number);
    bool get_bit(uint8_t bit_number);

private:
    Qmode1 qmode1;
    Qmode1 qmode4;
    int32_t qmode;
};

class Subcode {
public:
    Subcode();

    void begin_new_track(int32_t track_number, uint8_t _q_mode);
    void next_section();
    uint8_t get_subcode_byte(int32_t symbol_number);
    
private:
    uint8_t q_mode;
    uint8_t track_number;
    int32_t frame_number;

    //Pchannel pchannel;
    Qchannel qchannel;
    // Rchannel rchannel;
    // Schannel schannel;
    // Tchannel tchannel;
    // Uchannel uchannel;
    // Vchannel vchannel;
    // Wchannel wchannel;
};

#endif // SUBCODE_H