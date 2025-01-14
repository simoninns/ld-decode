/************************************************************************

    audio.cpp

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

#include <QFile>
#include <QByteArray>
#include <QDataStream>
#include <QVector>

#include "audio.h"

AudioToData::AudioToData(const QString &filename)
    : _filename(filename) {}

bool AudioToData::open() {
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
    return true;
}

void AudioToData::close() {
    audioData.clear();
}

QVector<uint8_t> AudioToData::read_24_bytes() {
    if (audioData.size() < 24) {
        return QVector<uint8_t>();
    }

    QVector<uint8_t> data;
    for (int i = 0; i < 24; ++i) {
        data.append(static_cast<uint8_t>(audioData[i]));
    }
    audioData.remove(0, 24);
    return data;
}

int AudioToData::frames_remaining() const {
    return audioData.size() / 4;
}