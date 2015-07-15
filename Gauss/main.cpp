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
#include <cmath>
#include <QCoreApplication>
#include <QImage>
#include <QTime>
#include <QElapsedTimer>
#include <QDebug>

template<typename T> class Pixel
{
    public:
        explicit Pixel():
            r(0), g(0), b(0)
        {
        }

        Pixel(T r, T g, T b):
            r(r), g(g), b(b)
        {
        }

        Pixel(QRgb pixel):
            r(qRed(pixel)), g(qGreen(pixel)), b(qBlue(pixel))
        {
        }

        template <typename R> Pixel &operator +=(const Pixel<R> &other)
        {
            this->r += other.r;
            this->g += other.g;
            this->b += other.b;

            return *this;
        }

        Pixel &operator /=(qreal c)
        {
            this->r /= c;
            this->g /= c;
            this->b /= c;

            return *this;
        }

        T r;
        T g;
        T b;
};

typedef Pixel<qint8> PixelI8;
typedef Pixel<quint8> PixelU8;
typedef Pixel<qreal> PixelReal;

template <typename T> inline Pixel<qreal> operator *(qreal c, const Pixel<T> &pixel)
{
    return Pixel<qreal>(c * pixel.r,
                        c * pixel.g,
                        c * pixel.b);
}

inline QVector<qreal> gaussKernel(int radius, qreal sigma, int *kl)
{
    int kw = 2 * radius + 1;
    QVector<qreal> kernel(kw * kw);
    qreal sum = 0;

    /* Create convolution matrix according to the formula:
     *
     *                    1             (-((i - radius) ^ 2 + (y - radius) ^ 2))
     * weight = -------------------- exp(--------------------------------------)
     *          2 * M_PI * sigma ^ 2    (             2 * sigma ^ 2            )
     *
     * Since the weights are normalized, the term:
     *
     *
     *                    1
     * weight = --------------------
     *          2 * M_PI * sigma ^ 2
     *
     * is not required here.
     */
    for (int j = 0; j < kw; j++)
        for (int i = 0; i < kw; i++) {
            int x = i - radius;
            int y = j - radius;
            qreal sigma2 = -2 * sigma * sigma;
            qreal weight = exp((x * x + y * y) / sigma2);
            kernel[i + j * kw] = weight;
            sum += weight;
        }

    // Normalize weights.
    for (int i = 0; i < kernel.size(); i++)
        kernel[i] /= sum;

    *kl = kw;

    return kernel;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    Q_UNUSED(a)

    QImage inImage("lena.png");
    inImage = inImage.convertToFormat(QImage::Format_RGB32);
    QImage outImage(inImage.size(), inImage.format());

    // Here we configure the denoise parameters.
    int radius = 3;
    qreal sigma = 1000;

    // Create gaussian denoise kernel.
    int kw;
    QVector<qreal> kernel = gaussKernel(radius, sigma, &kw);

    // Add noise to the image
    qsrand(QTime::currentTime().msec());

    for (int i = 0; i < 100000; i++) {
        inImage.setPixel(qrand() % inImage.width(),
                         qrand() % inImage.height(),
                         qRgb(qrand() % 256,
                              qrand() % 256,
                              qrand() % 256));
    }

    QElapsedTimer timer;
    timer.start();

    for (int y = 0; y < inImage.height(); y++) {
        const QRgb *iLine = (const QRgb *) inImage.constScanLine(y);
        QRgb *oLine = (QRgb *) outImage.scanLine(y);

        for (int x = 0; x < inImage.width(); x++) {
            PixelReal sum;
            qreal sumW = 0;

            // Apply kernel.
            for (int j = 0, pos = 0; j < kw; j++) {
                const QRgb *line = (const QRgb *) inImage.constScanLine(y + j - radius);

                if (y + j < radius
                    || y + j >= radius + inImage.height())
                    continue;

                for (int i = 0; i < kw; i++, pos++) {
                    if (x + i < radius
                        || x + i >= radius + inImage.width())
                        continue;

                    PixelU8 pixel(line[x + i - radius]);
                    qreal weight = kernel[pos];
                    sum += weight * pixel;
                    sumW += weight;
                }
            }

            // We normallize the kernel because the size of the kernel is not
            // fixed.
            sum /= sumW;

            oLine[x] = qRgba(sum.r, sum.g, sum.b, qAlpha(iLine[x]));
        }
    }

    qDebug() << timer.elapsed();
    outImage.save("gauss.png");

    return EXIT_SUCCESS;
}
