/* DenoiseFilters, Implementation of Gauss, Mean and Median filters in Qt/C++.
 * Copyright (C) 2015  Gonzalo Exequiel Pedone
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/DenoiseFilters
 */

#include <iostream>
#include <QCoreApplication>
#include <QImage>
#include <QTime>
#include <QElapsedTimer>
#include <QDebug>

class Buffer
{
    public:
        explicit Buffer():
            width(0),
            size(0)
        {
        }

        Buffer(const QImage &image)
        {
            this->width = image.width();
            this->size = this->width * image.height();
            this->r.resize(this->size);
            this->g.resize(this->size);
            this->b.resize(this->size);

            const QRgb *bits = (const QRgb *) image.constBits();

            for (int i = 0; i < this->size; i++) {
                QRgb pixel = bits[i];
                this->r[i] = qRed(pixel);
                this->g[i] = qGreen(pixel);
                this->b[i] = qBlue(pixel);
            }
        }

        QVector<const quint8 *> constPixel(int x, int y) const
        {
            QVector<const quint8 *> lines(3);
            lines[0] = this->r.constData() + x + y * this->width;
            lines[1] = this->g.constData() + x + y * this->width;
            lines[2] = this->b.constData() + x + y * this->width;

            return lines;
        }

        QVector<quint8> r;
        QVector<quint8> g;
        QVector<quint8> b;
        int width;
        int size;
};

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    Q_UNUSED(a)

    QImage inImage("lena.png");
    inImage = inImage.convertToFormat(QImage::Format_RGB32);
    QImage outImage(inImage.size(), inImage.format());

    // Here we configure the denoise parameters.
    int radius = 3;

    // Add noise to the image
    qsrand(QTime::currentTime().msec());

    for (int i = 0; i < 1000; i++) {
        inImage.setPixel(qrand() % inImage.width(),
                         qrand() % inImage.height(),
                         qRgb(qrand() % 256,
                              qrand() % 256,
                              qrand() % 256));
    }

    Buffer image(inImage);

    QElapsedTimer timer;
    timer.start();

    for (int y = 0; y < inImage.height(); y++) {
        QRgb *oLine = (QRgb *) outImage.scanLine(y);
        int yp = qMax(y - radius, 0);
        int kh = qMin(y + radius, inImage.height() - 1) - yp + 1;

        for (int x = 0; x < inImage.width(); x++) {
            int xp = qMax(x - radius, 0);
            int kw = qMin(x + radius, inImage.width() - 1) - xp + 1;

            int minR = 255;
            int minG = 255;
            int minB = 255;
            int maxR = 0;
            int maxG = 0;
            int maxB = 0;

            // Compare the pixels.
            for (int j = 0; j < kh; j++) {
                QVector<const quint8 *> pixel = image.constPixel(xp, yp + j);

                for (int i = 0; i < kw; i++) {
                    if (pixel[0][i] < minR)
                        minR = pixel[0][i];

                    if (pixel[0][i] > maxR)
                        maxR = pixel[0][i];

                    if (pixel[1][i] < minG)
                        minG = pixel[1][i];

                    if (pixel[1][i] > maxG)
                        maxG  = pixel[1][i];

                    if (pixel[2][i] < minB)
                        minB = pixel[2][i];

                    if (pixel[2][i] > maxB)
                        maxB  = pixel[2][i];
                }
            }

            quint8 r = (minR + maxR) / 2;
            quint8 g = (minG + maxG) / 2;
            quint8 b = (minB + maxB) / 2;

            oLine[x] = qRgb(r, g, b);
        }
    }

    qDebug() << timer.elapsed();
    outImage.save("pseudomedian.png");

    return EXIT_SUCCESS;
}
