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

#include "nervouselement.h"

NervousElement::NervousElement(): QbElement()
{
    this->m_convert = Qb::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    qRegisterMetaType<QRgb>("QRgb");

    this->resetNFrames();
    this->resetSimple();
    this->m_stride = 0;
}

int NervousElement::nFrames() const
{
    return this->m_nFrames;
}

bool NervousElement::simple() const
{
    return this->m_simple;
}

void NervousElement::setNFrames(int nFrames)
{
    this->m_nFrames = nFrames;
}

void NervousElement::setSimple(bool simple)
{
    this->m_simple = simple;
}

void NervousElement::resetNFrames()
{
    this->setNFrames(32);
}

void NervousElement::resetSimple()
{
    this->setSimple(false);
}

QbPacket NervousElement::iStream(const QbPacket &packet)
{
    QbPacket iPacket = this->m_convert->iStream(packet);
    QImage src = QbUtils::packetToImage(iPacket);

    if (src.isNull())
        return QbPacket();

    if (packet.caps() != this->m_caps) {
        this->m_frames.clear();
        this->resetNFrames();
        this->m_stride = 0;

        this->m_caps = packet.caps();
    }

    this->m_frames << src.copy();
    int diff = this->m_frames.size() - this->m_nFrames;

    for (int i = 0; i < diff; i++)
        this->m_frames.takeFirst();

    int timer = 0;
    int nFrame = 0;

    if (!this->m_simple) {
        if (timer) {
            nFrame += this->m_stride;
            nFrame = qBound(0, nFrame, this->m_frames.size() - 1);
            timer--;
        }
        else {
            nFrame = qrand() % this->m_frames.size();
            this->m_stride = qrand() % 5 - 2;

            if(this->m_stride >= 0)
                this->m_stride++;

            timer = qrand() % 6 + 2;
        }
    }
    else if(this->m_frames.size() > 0)
        nFrame = qrand() % this->m_frames.size();

    QImage oFrame = this->m_frames[nFrame];

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}
