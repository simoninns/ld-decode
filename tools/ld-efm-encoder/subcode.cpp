/************************************************************************

    subcode.cpp

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

#include "subcode.h"

Subcode::Subcode() {
    q_mode = 1; // Default to Compact Disc mode
    track_number = 0;  // Default to track 0
    symbol_number = 0;  // Default to symbol number 0
}

uint8_t Subcode::get_symbol(int32_t symbol_number, int32_t frame_number, int32_t abs_frame_number) {
    if (symbol_number < 2 || symbol_number > 97) {
        qFatal("Subcode::get_symbol(): Symbol number must be in the range 2 to 97.");
    }

    if (frame_number < 0) {
        qFatal("Subcode::get_symbol(): Frame number must be greater than or equal to zero.");
    }

    if (abs_frame_number < 0) {
        qFatal("Subcode::get_symbol(): Absolute frame number must be greater than or equal to zero.");
    }

    uint8_t symbol_byte = 0;

    if (get_p_channel_bit(symbol_number, frame_number)) symbol_byte |= 0x01;
    if (get_q_channel_bit(symbol_number, frame_number)) symbol_byte |= 0x02;
    if (get_r_channel_bit(symbol_number, frame_number)) symbol_byte |= 0x04;
    if (get_s_channel_bit(symbol_number, frame_number)) symbol_byte |= 0x08;
    if (get_t_channel_bit(symbol_number, frame_number)) symbol_byte |= 0x10;
    if (get_u_channel_bit(symbol_number, frame_number)) symbol_byte |= 0x20;
    if (get_v_channel_bit(symbol_number, frame_number)) symbol_byte |= 0x40;
    if (get_w_channel_bit(symbol_number, frame_number)) symbol_byte |= 0x80;

    return symbol_byte;
}

void Subcode::begin_new_track(int32_t _track_number, uint8_t _q_mode) {
    if (_track_number < 1 || _track_number > 99) {
        qFatal("Subcode::begin_new_track(): Track number must be in the range 1 to 99.");
    }
    track_number = _track_number;

    // Q mode 0: Custom DATA-Q - Unsupported
    // Q mode 1: Compact Disc
    // Q mode 2: Catalogue number - Unsuppoerted
    // Q mode 3: Track ID - Unsupported
    // Q mode 4: LaserDisc
    //
    // Note that Q modes 1 and 4 are basically the same as far as the subcode is concerned
    if (_q_mode < 0 || _q_mode > 4) {
        qFatal("Subcode::begin_new_track(): Q mode must be in the range 0 to 4.");
    }
    q_mode = _q_mode;

    // Right now we will only support Q modes 1 and 4
    if (q_mode != 1 && q_mode != 4) {
        qFatal("Subcode::begin_new_track(): Q mode must be 1 or 4.");
    }
}

bool Subcode::get_p_channel_bit(uint8_t _symbol_number, int32_t _frame_number) {
    if (_symbol_number < 2 || _symbol_number > 97) {
        qFatal("Subcode::get_p_channel_bit(): Symbol number must be in the range 2 to 97.");
    }
    
    Pchannel pchannel(_frame_number);

    // Stuff goes here...

    // Symbol is 0-98, but 2 symbols are consumed by the sync0 and sync1 symbols
    // So the bit number is the symbol number minus 2
    uint8_t bit_number = _symbol_number - 2;

    return pchannel.get_bit(bit_number);
}

bool Subcode::get_q_channel_bit(uint8_t _symbol_number, int32_t _frame_number) {
    if (_symbol_number < 2 || _symbol_number > 97) {
        qFatal("Subcode::get_q_channel_bit(): Symbol number must be in the range 2 to 97.");
    }
    
    // Generate the QMode1 object for the frame
    Qmode1 qmode1(_frame_number, track_number);
    qmode1.set_as_audio();

    // Symbol is 0-98, but 2 symbols are consumed by the sync0 and sync1 symbols
    // So the bit number is the symbol number minus 2
    uint8_t bit_number = _symbol_number - 2;

    return qmode1.get_bit(bit_number);
}

// Qchannel class implementation --------------------------------------------------------------------------------------
Qchannel::Qchannel(int32_t _frame_number) : frame_number(_frame_number) {
    // Initialize the channel data to all zeros
    channel_data.resize(12);
    std::fill(channel_data.begin(), channel_data.end(), 0);
}

bool Qchannel::get_bit(uint8_t bit_number) {
    if (bit_number < 0 || bit_number > 96) {
        qFatal("Qchannel::get_bit(): Bit number must be in the range 0 to 96.");
    }

    // We have a QByteArray of 12 bytes, each byte contains 8 bits
    // The bit number is the bit position in the 96-bit subcode data
    // The byte number is the byte position in the 12-byte subcode data
    int byte_number = bit_number / 8;
    int bit_position = bit_number % 8;

    return (channel_data[byte_number] & (1 << bit_position));
}

// Generate a 16-bit CRC for the subcode data
// Adapted from http://mdfs.net/Info/Comp/Comms/CRC16.htm
uint16_t Qchannel::crc16(const uchar *addr, uint16_t num)
{
    int32_t i;
    uint32_t crc = 0;

    for (; num > 0; num--) {
        crc = crc ^ static_cast<uint32_t>(*addr++ << 8);
        for (i = 0; i < 8; i++) {
            crc = crc << 1;
            if (crc & 0x10000) crc = (crc ^ 0x1021) & 0xFFFF;
        }
    }

    return static_cast<uint16_t>(crc);
}

// Convert integer to BCD (Binary Coded Decimal)
// Output is always 2 bytes (00-99)
uint32_t Qchannel::int_to_bcd2(uint32_t value) {
    if (value > 99) {
        qFatal("Qchannel::int_to_bcd2(): Value must be in the range 0 to 99.");
    }

    uint32_t bcd = 0;
    uint32_t factor = 1;

    while (value > 0) {
        bcd += (value % 10) * factor;
        value /= 10;
        factor *= 16;
    }

    // Ensure the result is always 2 bytes (00-99)
    return bcd & 0xFF;
}

// Qmode1 class implementation ----------------------------------------------------------------------------------------
Qmode1::Qmode1(int32_t _frame_number, int32_t _track_number) : Qchannel(_frame_number) {
    // Set the current track number
    // Note: Track 00 is the lead-in track and track 99 is the lead-out track
    // Track numbers 01-98 are the audio tracks
    if (_track_number < 0 || _track_number > 99) {
        qFatal("Qmode1::Qmode1(): Track number must be in the range 0 to 99.");
    }
    track_number = _track_number;

    if (track_number < 1) {
        // Lead in
        set_as_lead_in();
    } else {
        // Audio
        set_as_audio();
    }
}

// Set the QMode1 object to audio mode
void Qmode1::set_as_audio() {
    if (!is_audio) {
        is_audio = true;
        generate_audio();
    }
}

// Set the QMode1 object to lead-in mode
void Qmode1::set_as_lead_in() {
    if (is_audio) {
        is_audio = false;
        track_number = 0;
        generate_lead_in();
    }
}

// Set the QMode1 object to lead-out mode
void Qmode1::set_as_lead_out() {
    if (is_audio) {
        is_audio = true;
        track_number = 99;
        generate_audio();
    }
}

// Generate the QMode1 audio data for the current frame
void Qmode1::generate_audio() {
    // Initialize the channel data to all zeros
    channel_data.resize(12);
    std::fill(channel_data.begin(), channel_data.end(), 0);

    generate_common();

    // X field: (X)
    // 00: Encoder paused
    // 01: Encoder running
    //
    // Note: This is a simplification - see ECMA-130 for the full details
    qmode1_audio.x_field = 0x01;

    // AMIN, ASEC, AFRAME
    // This is the running time on disc... right now this is wrong
    // as we don't have an absolute frame number reference...
    qmode1_audio.amin_field = int_to_bcd2(frame_number / 4500);
    qmode1_audio.asec_field = int_to_bcd2((frame_number % 4500) / 75);
    qmode1_audio.aframe_field = int_to_bcd2((frame_number % 4500) % 75);

    // Copy the audio fields into the channel data...

    // X is 1 byte
    channel_data[2] = qmode1_audio.x_field;

    // AMIN, ASEC, AFRAME are 1 byte each (00-99 in BCD)
    channel_data[7] = qmode1_audio.amin_field;
    channel_data[8] = qmode1_audio.asec_field;
    channel_data[9] = qmode1_audio.aframe_field;

    // Generate the CRC for the channel data
    generate_crc();
}

// Generate the QMode1 lead-in data for the current frame
void Qmode1::generate_lead_in() {
    // Initialize the channel data to all zeros
    channel_data.resize(12);
    std::fill(channel_data.begin(), channel_data.end(), 0);

    generate_common();

    // POINT field: (Point)
    qmode1_lead_in.point_field = 0;

    // PMIN, PSEC, PFRAME
    // This is completely wrong... it's really a TOC reference
    qmode1_lead_in.pmin_field = int_to_bcd2(frame_number / 4500);
    qmode1_lead_in.psec_field = int_to_bcd2((frame_number % 4500) / 75);
    qmode1_lead_in.pframe_field = int_to_bcd2((frame_number % 4500) % 75);

    // Copy the lead-in fields into the channel data...

    // POINT is 1 byte
    channel_data[2] = qmode1_lead_in.point_field;

    // AMIN, ASEC, AFRAME are 1 byte each (00-99 in BCD)
    channel_data[7] = qmode1_lead_in.pmin_field;
    channel_data[8] = qmode1_lead_in.psec_field;
    channel_data[9] = qmode1_lead_in.pframe_field;

    // Generate the CRC for the channel data
    generate_crc();
}

void Qmode1::generate_common() {
    // Control field: (CONTROL)
    //
    // Bits 8 4 2 1
    //      x 0 0 0 = 0 is 2-Channel, 1 is 4-Channel
    //      0 x 0 0 = 0 is audio, 1 is data
    //      0 0 x 0 = 0 is copy not permitted, 1 is copy permitted
    //      0 0 0 x = 0 is pre-emphasis off, 1 is pre-emphasis on
    qmode1_common.control_field = 0; // 2-Channel, Audio, Copy not permitted, Pre-emphasis off

    // Address field: (ADR)
    //
    // 0000: ADR 0, mode 0 for DATA-Q
    // 0001: ADR 1, mode 1 for DATA-Q
    // 0010: ADR 2, mode 2 for DATA-Q
    // 0011: ADR 3, mode 3 for DATA-Q
    // 0100: ADR 4, mode 4 for DATA-Q
    qmode1_common.adr_field = 0x20; // Mode 1 for DATA-Q

    // TNO field: (Track Number BCD)
    // 00: Lead-in
    // 01-99: Track number
    // AA: Lead-out
    qmode1_common.tno_field = int_to_bcd2(track_number);

    // MIN, SEC, FRAME
    // This is based on the current frame number
    // with frame number 0 being the first frame
    // and each frame being 1/75th of a second
    qmode1_common.min_field = int_to_bcd2(frame_number / 4500);
    qmode1_common.sec_field = int_to_bcd2((frame_number % 4500) / 75);
    qmode1_common.frame_field = int_to_bcd2((frame_number % 4500) % 75);

    // ZERO field:
    // 8-bit field set to zero
    qmode1_common.zero_field = 0;

    // Copy the common fields into the channel data...

    // CONTROL is 4 bits, ADR is 4 bits. Combine them into a single byte
    channel_data[0] = (qmode1_common.control_field << 4) | qmode1_common.adr_field;

    // TNO is 1 byte (00-99 in BCD)
    channel_data[1] = qmode1_common.tno_field;

    // POINT or X uses 1 byte here

    // MIN, SEC, FRAME are 1 byte each (00-99 in BCD)
    channel_data[3] = qmode1_common.min_field;
    channel_data[4] = qmode1_common.sec_field;
    channel_data[5] = qmode1_common.frame_field;

    // ZERO is 1 byte
    channel_data[6] = qmode1_common.zero_field;
}

void Qmode1::generate_crc() {
    // Generate the CRC for the channel data
    uint16_t crc = crc16(reinterpret_cast<const uchar *>(channel_data.data()), 10);

    // CRC is 2 bytes
    channel_data[10] = crc >> 8;
    channel_data[11] = crc & 0xFF;
}

// Pchannel class implementation --------------------------------------------------------------------------------------
Pchannel::Pchannel(int32_t _frame_number) : frame_number(_frame_number) {
    // Initialize the channel data to all zeros
    channel_data.resize(12);
    std::fill(channel_data.begin(), channel_data.end(), 0);
}

bool Pchannel::get_bit(uint8_t bit_number) {
    if (bit_number < 0 || bit_number > 96) {
        qFatal("Qchannel::get_bit(): Bit number must be in the range 0 to 96.");
    }

    // We have a QByteArray of 12 bytes, each byte contains 8 bits
    // The bit number is the bit position in the 96-bit subcode data
    // The byte number is the byte position in the 12-byte subcode data
    int byte_number = bit_number / 8;
    int bit_position = bit_number % 8;

    return (channel_data[byte_number] & (1 << bit_position));
}