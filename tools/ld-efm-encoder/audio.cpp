/************************************************************************

    audio.cpp

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

#include <QFile>
#include <QByteArray>
#include <QDataStream>
#include <QVector>
#include <QDebug>

#include "audio.h"

AudioToData::AudioToData(const QString &filename, bool audio_test, int32_t audio_test_frames) : _filename(filename), _audio_test(audio_test), _audio_test_frames(audio_test_frames) {}

bool AudioToData::open() {
    if (!_audio_test) {
        QFile file(_filename);
        if (!file.open(QIODevice::ReadOnly)) {
            return false;
        }

        QByteArray header = file.read(44);
        if (header.size() != 44) {
            return false;
        }

        // Check WAV file format
        if (header.mid(0, 4) != "RIFF" || header.mid(8, 4) != "WAVE" ||
            header.mid(12, 4) != "fmt " || header.mid(20, 2).toHex() != "0100" ||
            header.mid(22, 2).toHex() != "0200" || header.mid(24, 4).toHex() != "44ac0000" ||
            header.mid(34, 2).toHex() != "1000") {
            return false;
        }

        audioData = file.readAll();
        file.close();
    } else {
        // This an audio test, so we generate the audio data based
        // on the number of frames requested

        // Note: The 16 bit samples (according to IEC 60908-1999 are 16-bit signed integers)
        // Order is left channel, right channel, left channel, right channel, etc.
        // 
        // Data is big-endian, so the first byte is the most significant byte and the first bit
        // is the most significant bit

        audioData.clear();
        int16_t left_sample = 0;
        int16_t right_sample = 0;

        qInfo() << "AudioToData::open(): Generating audio test data with" << _audio_test_frames << "frames.";
        qInfo() << "AudioToData::open(): Audio test data will be 16-bit stereo at 44.1kHz totalling" << _audio_test_frames * 4 << "bytes.";
        qInfo() << "AudioToData::open():" << _audio_test_frames << "frames contains" << _audio_test_frames * 2 << "samples.";

        for (uint32_t i = 0; i < _audio_test_frames; ++i) {
            uint8_t left_msb = static_cast<uint8_t>((left_sample >> 8) & 0xFF);
            uint8_t left_lsb = static_cast<uint8_t>(left_sample & 0xFF);
            uint8_t right_msb = static_cast<uint8_t>((right_sample >> 8) & 0xFF);
            uint8_t right_lsb = static_cast<uint8_t>(right_sample & 0xFF);

            audioData.append(left_msb);
            audioData.append(left_lsb);
            audioData.append(right_msb);
            audioData.append(right_lsb);

            // Change the sample values
            left_sample++;
            right_sample--;
        }
    }

    // The longest possible delay of data through the encoder (according to ECMA-130 issue 2 page 34) is
    // 108 F1 Frames (the shortest is 3 F1 Frames).  Since an F1 frame is 24 8-bit bytes we need to pad
    // the incoming data by 108 * 24 bytes to ensure we have enough data to process = 2592 bytes
    // Otherwise we will loose audio data at the end of the file
    audioData.append(QByteArray(2592, 0));

    qInfo() << "AudioToData::open(): Audio data loaded with" << audioData.size() << "bytes after padding";

    // // Debug output of audioData contents in xxd format
    // qDebug() << "AudioToData::open(): Audio data contents:";
    // for (int i = 0; i < audioData.size(); i += 16) {
    //     QString line = QString("%1: ").arg(i, 8, 16, QChar('0'));
    //     for (int j = 0; j < 16 && i + j < audioData.size(); ++j) {
    //         line += QString("%1 ").arg(static_cast<uint8_t>(audioData[i + j]), 2, 16, QChar('0'));
    //     }
    //     qDebug().noquote() << line;
    // }

    return true;
}

void AudioToData::close() {
    audioData.clear();
}

QVector<uint8_t> AudioToData::read_24_bytes() {
    if (audioData.size() < 24) {
        return QVector<uint8_t>();
    }

    QVector<uint8_t> data = QVector<uint8_t>(audioData.begin(), audioData.begin() + 24);
    audioData.remove(0, 24);
    return data;
}

int AudioToData::frames_remaining() const {
    return audioData.size() / 4;
}