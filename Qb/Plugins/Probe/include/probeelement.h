/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2014  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

#ifndef PROBEELEMENT_H
#define PROBEELEMENT_H

#include <qbelement.h>

class ProbeElement: public QbElement
{
    Q_OBJECT
    Q_PROPERTY(bool log READ log WRITE setLog RESET resetLog)

    public:
        explicit ProbeElement();

        Q_INVOKABLE bool log() const;

    private:
        bool m_log;

    public slots:
        void setLog(bool log);
        void resetLog();

        QbPacket iStream(const QbPacket &packet);
};

#endif // PROBEELEMENT_H
