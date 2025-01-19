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

AudioToData::AudioToData(const QString filename) : _filename(filename), file(filename) {}

bool AudioToData::open() {
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    qWarning() << "Failed to open file:" << _filename;
    }

    return true;
}

void AudioToData::close() {
    file.close();
}

QVector<uint8_t> AudioToData::read_24_bytes() {
    // Read the 24 bytes of audio data from the file
    QVector<uint8_t> data(24);
    qint64 bytesRead = file.read(reinterpret_cast<char*>(data.data()), 24);

    // If we have less than 24 bytes, we have reached the end of the file
    if (bytesRead < 24) {
        return QVector<uint8_t>();
    }

    return data;
}