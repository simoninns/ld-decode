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
    // Initialise the subcode object
    begin_new_track(1, 1); // Track #1, Q mode 1 (CD audio)
}

void Subcode::begin_new_track(int32_t _track_number, uint8_t _q_mode) {
    if (_track_number < 1 || _track_number > 99) {
        qFatal("Subcode::begin_new_track(): Track number must be in the range 1 to 99.");
    }
    track_number = _track_number;
    frame_number = 0;

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

    // Generate the subcode data
    qchannel.generate_frame(q_mode, track_number, frame_number, frame_number);
    pchannel.generate_frame(false);
}

void Subcode::next_section() {
    frame_number++;

    // Generate the subcode data
    qchannel.generate_frame(q_mode, track_number, frame_number, frame_number);
}

uint8_t Subcode::get_subcode_byte(int32_t symbol_number) {
    if (symbol_number < 2 || symbol_number > 98) {
        qFatal("Subcode::get_subcode_byte(): Symbol number must be in the range 2 to 98.");
    }

    // Symbol number is which one of the frame's symbols we want to get from 0-98

    symbol_number -= 2; // Convert to 0-based index

    uint8_t subcode_byte = 0;
    // if (pchannel.get_bit(symbol_number)) subcode_byte |= 0x80;
    if (qchannel.get_bit(symbol_number)) subcode_byte |= 0x40;

    // ECMA-130 page 20 - Clause 22.1 states that channels r to w are reserved
    // and should be set to zero

    return subcode_byte;
}

// Pchannel class implementation --------------------------------------------------------------------------------------
Pchannel::Pchannel() {
    // Initialise to some happy defaults
    generate_frame(false);
}

void Pchannel::generate_frame(bool _flag) {
    // The P channel is a single bit that is set to 1 if the Q channel is in Q mode 1
    // and 0 if the Q channel is in Q mode 4
    if (_flag) {
        // If the flag is true, set the P channel data to 1s
        channel_data.fill(0xFF, 12);
    } else {
        // If the flag is true, set the P channel data to 0s
        channel_data.fill(0xFF, 12);
    }
}

bool Pchannel::get_bit(uint8_t symbol_number) {
    if (symbol_number < 0 || symbol_number > 96) {
        qFatal("Pchannel::get_bit(): Bit number must be in the range 0 to 96.");
    }

    // The symbol number is the bit position in the 96-bit subcode data
    // We need to convert this to a byte number and bit number within that byte
    uint8_t byte_number = symbol_number / 8;
    uint8_t bit_number = 7 - (symbol_number % 8); // Change to make MSB at the other end

    uint8_t sc_byte = channel_data[byte_number];

    // Return true or false based on the required bit
    return (sc_byte & (1 << bit_number)) != 0;
}

// Qchannel class implementation --------------------------------------------------------------------------------------
Qchannel::Qchannel() : qmode1(1), qmode4(4) {
    // Initialise to some happy defaults
    generate_frame(1, 1, 1, 1);
}

void Qchannel::generate_frame(uint8_t _qmode, int32_t _track_number, int32_t _frame_number, int32_t _absolute_frame_number) {
    if (_qmode != 1 && _qmode != 4) {
        qFatal("Qchannel::generate_frame(): Q mode must be 1 or 4.");
    }

    if (_track_number < 1 || _track_number > 98) {
        qFatal("Qchannel::generate_frame(): Track number must be in the range 1 to 98.");
    }

    if (_frame_number < 0) {
        qFatal("Qchannel::generate_frame(): Frame number must be greater than or equal to zero.");
    }

    if (_absolute_frame_number < 0) {
        qFatal("Qchannel::generate_frame(): Absolute frame number must be greater than or equal to zero.");
    }

    if (_qmode == 1) {
        qmode1.configure_frame(Qmode1and4::AUDIO, _track_number, _frame_number, _absolute_frame_number);
        qmode = 1;
    } else if (_qmode == 4) {
        qmode4.configure_frame(Qmode1and4::AUDIO, _track_number, _frame_number, _absolute_frame_number);
        qmode = 4;
    }
}

bool Qchannel::get_bit(uint8_t symbol_number) {
    if (symbol_number < 0 || symbol_number > 96) {
        qFatal("Qchannel::get_bit(): Bit number must be in the range 0 to 96.");
    }

    // The symbol number is the bit position in the 96-bit subcode data
    // We need to convert this to a byte number and bit number within that byte
    uint8_t byte_number = symbol_number / 8;
    uint8_t bit_number = 7 - (symbol_number % 8); // Change to make MSB at the other end

    uint8_t sc_byte = 0;
    if (qmode == 1) {
        sc_byte = qmode1.get_byte(byte_number);
    } else if (qmode == 4) {
        sc_byte = qmode4.get_byte(byte_number);
    } else {
        qFatal("Qchannel::get_bit(): Q mode must be 1 or 4.");
    }

    // Return true or false based on the required bit
    return (sc_byte & (1 << bit_number)) != 0;
}

// Qmode1and4 class implementation ----------------------------------------------------------------------------------------
Qmode1and4::Qmode1and4(int32_t _qmode) : qmode(_qmode) {
    // Verify the Q mode
    if (qmode != 1 && qmode != 4) {
        qFatal("Qmode1and4::Qmode1and4(): Q mode must be 1 or 4.");
    }
    // Initialize the channel data to all zeros
    channel_data.resize(12);
    std::fill(channel_data.begin(), channel_data.end(), 0);

    // Set up some default data
    configure_frame(AUDIO, 1, 1, 1);
}

void Qmode1and4::configure_frame(FrameType frame_type, int32_t _track_number, int32_t _frame_number, int32_t _absolute_frame_number) {
    if (frame_type == AUDIO) {
        track_number = _track_number;
        frame_number = _frame_number;
        absolute_frame_number = _absolute_frame_number;
        generate_audio();
    } else if (frame_type == LEAD_IN) {
        track_number = 0;
        frame_number = _frame_number;
        absolute_frame_number = _absolute_frame_number;
        generate_lead_in();
    } else if (frame_type == LEAD_OUT) {
        track_number = 99;
        frame_number = _frame_number;
        absolute_frame_number = _absolute_frame_number;
        generate_audio();
    } else {
        qFatal("Qmode1and4::configure_frame(): Invalid frame type.");
    }
}

uint8_t Qmode1and4::get_byte(int32_t byte_number) {
    return channel_data[byte_number];
}

// Generate the QMode1 audio data for the current frame
void Qmode1and4::generate_audio() {
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

    // Generate the CRC for the qchannel data
    generate_crc();

    // // Output the qchannel data into qDebug
    // QString binary_output;
    // for (int i = 0; i < channel_data.size(); ++i) {
    //     binary_output.append(QString::number(static_cast<uint8_t>(channel_data[i]), 2).rightJustified(8, '0'));
    //     if (i < channel_data.size() - 1) {
    //         binary_output.append(" ");
    //     }
    // }
    // qDebug() << "Q-channel Data (binary): CON ADR  TNO      X        MIN      SEC      FRAME    ZERO     AMIN     ASEC     AFRAME   CRC16";
    // qDebug().noquote() << "Q-channel Data (binary):" << binary_output;
}

// Generate the QMode1 lead-in data for the current frame
void Qmode1and4::generate_lead_in() {
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

void Qmode1and4::generate_common() {
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
    qmode1_common.adr_field = 0x04; // Mode 4 for DATA-Q

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

    // CONTROL is 4 bits, ADR is 4 bits. Combine them into a single byte with CONTROL in the high nibble
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

void Qmode1and4::generate_crc() {
    // Generate the CRC for the channel data
    // CRC is on control+mode+data 4+4+72 = 80 bits with 16-bit CRC (96 bits total)

    uint16_t crc = crc16(channel_data.mid(0, 10));

    // Invert the CRC
    crc = ~crc & 0xFFFF;

    // CRC is 2 bytes
    channel_data[10] = crc >> 8;
    channel_data[11] = crc & 0xFF;
}

// Generate a 16-bit CRC for the subcode data
// Adapted from http://mdfs.net/Info/Comp/Comms/CRC16.htm
uint16_t Qmode1and4::crc16(const QByteArray &data)
{
    int32_t i;
    uint32_t crc = 0;

    for (int pos = 0; pos < data.size(); pos++) {
        crc = crc ^ static_cast<uint32_t>(static_cast<uchar>(data[pos]) << 8);
        for (i = 0; i < 8; i++) {
            crc = crc << 1;
            if (crc & 0x10000) crc = (crc ^ 0x1021) & 0xFFFF;
        }
    }

    return static_cast<uint16_t>(crc);
}

// Convert integer to BCD (Binary Coded Decimal)
// Output is always 2 bytes (00-99)
uint32_t Qmode1and4::int_to_bcd2(uint32_t value) {
    if (value > 99) {
        qFatal("Qmode1and4::int_to_bcd2(): Value must be in the range 0 to 99.");
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

uint32_t Qmode1and4::bcd2_to_int(uint32_t bcd) {
    uint32_t value = 0;
    uint32_t factor = 1;

    while (bcd > 0) {
        value += (bcd & 0x0F) * factor;
        bcd >>= 4;
        factor *= 10;
    }

    return value;
}