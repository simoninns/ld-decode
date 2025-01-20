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

class Subcode {
public:
    Subcode();

    uint8_t get_symbol(int32_t symbol_number, int32_t frame_number, int32_t abs_frame_number);
    void begin_new_track(int32_t track_number, uint8_t _q_mode);
    
private:
    bool get_p_channel_bit(uint8_t symbol_number, int32_t index);
    bool get_q_channel_bit(uint8_t symbol_number, int32_t index);

    // The following channels are not implemented:
    bool get_r_channel_bit(int32_t symbol_number, int32_t index) { return false; };
    bool get_s_channel_bit(int32_t symbol_number, int32_t index) { return false; };
    bool get_t_channel_bit(int32_t symbol_number, int32_t index) { return false; };
    bool get_u_channel_bit(int32_t symbol_number, int32_t index) { return false; };    
    bool get_v_channel_bit(int32_t symbol_number, int32_t index) { return false; };
    bool get_w_channel_bit(int32_t symbol_number, int32_t index) { return false; };

    uint16_t crc16(const uchar *addr, uint16_t num);
    uint32_t int_to_bcd(uint32_t value);

    uint8_t q_mode;
    uint8_t track_number;
    int32_t symbol_number;
};

// Q Channel classes
class Qchannel {
public:
    Qchannel(int32_t _frame_number);
    bool get_bit(uint8_t bit_number);

protected:
    QByteArray channel_data;
    int32_t frame_number;

    uint16_t crc16(const uchar *addr, uint16_t num);
    uint32_t int_to_bcd2(uint32_t value);
};

class Qmode1 : public Qchannel {
public:
    Qmode1(int32_t _frame_number, int32_t _track_number);
    void set_as_audio();
    void set_as_lead_in();
    void set_as_lead_out();

private:
    bool is_audio;
    int32_t track_number;

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
};

// QMode 4 is the same as QMode 1
class Qmode4 : public Qchannel {  
public:
    Qmode4(int32_t _frame_number) : Qchannel(_frame_number) {}
};

// P Channel classes
class Pchannel {
public:
    Pchannel(int32_t _frame_number);
    bool get_bit(uint8_t symbol_number);

protected:
    QByteArray channel_data;
    int32_t frame_number;
};

#endif // SUBCODE_H