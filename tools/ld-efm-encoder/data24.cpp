/************************************************************************

    data24.cpp

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

#include "data24.h"

Data24::Data24(const QString _filename, bool _is_wav) : filename(_filename), file(_filename), is_wav(_is_wav) {
    ready = false;

    // Open the data file
    open();
}

Data24::~Data24() {
    file.close();
    ready = false;
}

bool Data24::open() {
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Data24::Data24() - Failed to open file:" << filename;
        return false;
    }

    ready = true;
    return true;
}

QVector<uint8_t> Data24::read() {
    // Read the 24 bytes of data from the file
    QVector<uint8_t> data(24);
    qint64 bytesRead = file.read(reinterpret_cast<char*>(data.data()), 24);

    // If no bytes were read, return an empty vector
    if (bytesRead == 0) {
        ready = false;
        last_frame.clear();
        return QVector<uint8_t>();
    }

    // If there are less than 24 bytes read, pad data with zeros to 24 bytes
    if (bytesRead < 24) {
        qDebug() << "Data24::read() - Read" << bytesRead << "bytes from file:" << filename << "padding with" << 24-bytesRead << "zeros";
        data.resize(24);
        for (int i = bytesRead; i < 24; ++i) {
            data[i] = 0;
        }
    }

    last_frame = data;
    return data;
}

bool Data24::is_ready() const {
    return ready;
}

void Data24::show_data() {
    if (last_frame.isEmpty()) {
        qInfo() << "Input data: No data available";
        return;
    }

    QString dataString;
    for (int i = 0; i < last_frame.size(); ++i) {
        dataString.append(QString("%1 ").arg(last_frame[i], 2, 16, QChar('0')));
    }
    qInfo().noquote() << "Input data:" << dataString.trimmed();
}