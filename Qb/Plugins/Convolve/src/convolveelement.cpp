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

#include "convolveelement.h"
#include "defs.h"

ConvolveElement::ConvolveElement(): QbElement()
{
    this->m_convert = Qb::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    this->resetKernel();
    this->resetKernelSize();
    this->resetFactor();
    this->resetBias();
}

QVariantList ConvolveElement::kernel() const
{
    QVariantList kernel;

    foreach (int e, this->m_kernel)
        kernel << e;

    return kernel;
}

QSize ConvolveElement::kernelSize() const
{
    return this->m_kernelSize;
}

QbFrac ConvolveElement::factor() const
{
    return this->m_factor;
}

int ConvolveElement::bias() const
{
    return this->m_bias;
}

void ConvolveElement::setKernel(const QVariantList &kernel)
{
    this->m_kernel.clear();

    foreach (QVariant e, kernel)
        this->m_kernel << e.toInt();
}

void ConvolveElement::setKernelSize(const QSize &kernelSize)
{
    this->m_kernelSize = kernelSize;
}

void ConvolveElement::setFactor(const QbFrac &factor)
{
    this->m_factor = factor;
}

void ConvolveElement::setBias(int bias)
{
    this->m_bias = bias;
}

void ConvolveElement::resetKernel()
{
    this->m_kernel.clear();

    this->m_kernel << 0 << 0 << 0
                   << 0 << 1 << 0
                   << 0 << 0 << 0;
}

void ConvolveElement::resetKernelSize()
{
    this->setKernelSize(QSize(3, 3));
}

void ConvolveElement::resetFactor()
{
    this->setFactor(QbFrac(0, 0));
}

void ConvolveElement::resetBias()
{
    this->setBias(0);
}

QbPacket ConvolveElement::iStream(const QbPacket &packet)
{
    QbPacket iPacket = this->m_convert->iStream(packet);
    QImage src = QbUtils::packetToImage(iPacket);

    if (src.isNull())
        return QbPacket();

    QImage oFrame(src.size(), src.format());
    Pixel *iPixel = (Pixel *) src.bits();
    quint8 *oPixel = (quint8 *) oFrame.bits();
    Pixel *srcBits = iPixel;

    int srcWidth = src.width();
    int srcHeight = src.height();
    int *kernelBits = const_cast<int *>(this->m_kernel.data());
    int factorNum = this->m_factor.num();
    int factorDen = this->m_factor.den();
    int minI;
    int maxI;
    int minJ;
    int maxJ;
    int kernelWidth = this->m_kernelSize.width();
    int kernelHeight = this->m_kernelSize.height();
    int dxMin = (kernelWidth - 1) >> 1;
    int dxMax = (kernelWidth + 1) >> 1;
    int dyMin = (kernelHeight - 1) >> 1;
    int dyMax = (kernelHeight + 1) >> 1;
    int sw = kernelWidth + srcWidth;
    int sh = kernelHeight + srcHeight;

    for (int y = 0; y < srcHeight; y++) {
        int minY = y - dyMin;
        int maxY = y + dyMax;

        if (minY < 0) {
            minJ = ABS(minY);
            minY = 0;
        }
        else
            minJ = 0;

        if (maxY > srcHeight) {
            maxJ = sh - maxY;
            maxY = srcHeight;
        }
        else
            maxJ = kernelHeight;

        int *k0 = kernelBits + minJ * kernelWidth;
        Pixel *src0 = srcBits + minY * srcWidth;

        for (int x = 0; x < srcWidth; x++) {
            int minX = x - dxMin;
            int maxX = x + dxMax;

            if (minX < 0) {
                minI = ABS(minX);
                minX = 0;
            }
            else
                minI = 0;

            if (maxX > srcWidth) {
                maxI = sw - maxX;
                maxX = srcWidth;
            }
            else
                maxI = kernelWidth;

            int diffX = srcWidth - maxX + minX;
            int diffI = kernelWidth - maxI - minI;

            int r = 0;
            int g = 0;
            int b = 0;

            int *k = k0 + minI;
            Pixel *src = src0 + minX;

            for (int j = minJ; j < maxJ; j++, k += diffI, src += diffX)
                for (int i = minI; i < maxI; i++, k++, src++)
                    if (*k) {
                        r += *k * src->r;
                        g += *k * src->g;
                        b += *k * src->b;
                    }

            if (factorNum) {
                r = factorNum * r / factorDen + this->m_bias;
                g = factorNum * g / factorDen + this->m_bias;
                b = factorNum * b / factorDen + this->m_bias;
            }

            *oPixel++ = bound(0, b, 255);
            *oPixel++ = bound(0, g, 255);
            *oPixel++ = bound(0, r, 255);
            *oPixel++ = iPixel->a;

            iPixel++;
        }
    }

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}
