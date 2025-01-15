/************************************************************************

    audio.h

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

#ifndef AUDIO_H
#define AUDIO_H

#include <QVector>
#include <QString>
#include <QByteArray>

class AudioToData {
public:
    AudioToData(const QString &filename, bool audio_test, int32_t audio_test_frames);
    bool open();
    void close();
    QVector<uint8_t> read_24_bytes();
    int frames_remaining() const;

private:
    QString _filename;
    QByteArray audioData;
    bool _audio_test;
    int32_t _audio_test_frames;
};

#endif // AUDIO_H
