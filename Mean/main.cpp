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

        Pixel &operator =(QRgb pixel)
        {
            this->r = qRed(pixel);
            this->g = qGreen(pixel);
            this->b = qBlue(pixel);

            return *this;
        }

        Pixel operator +(const Pixel &other) const
        {
            return Pixel(this->r + other.r,
                         this->g + other.g,
                         this->b + other.b);
        }

        Pixel operator +(int c) const
        {
            return Pixel(this->r + c,
                         this->g + c,
                         this->b + c);
        }

        template <typename R> Pixel operator -(const Pixel<R> &other) const
        {
            return Pixel(this->r - other.r,
                         this->g - other.g,
                         this->b - other.b);
        }

        template <typename R> Pixel operator *(const Pixel<R> &other) const
        {
            return Pixel(this->r * other.r,
                         this->g * other.g,
                         this->b * other.b);
        }

        template <typename R> Pixel<R> operator /(R c) const
        {
            return Pixel<R>(this->r / c,
                            this->g / c,
                            this->b / c);
        }

        template <typename R> Pixel operator /(const Pixel<R> &pixel) const
        {
            return Pixel(this->r / pixel.r,
                         this->g / pixel.g,
                         this->b / pixel.b);
        }

        Pixel operator <<(int bits) const
        {
            return Pixel(this->r << bits,
                         this->g << bits,
                         this->b << bits);
        }

        template <typename R> Pixel operator |(const Pixel<R> &other) const
        {
            return Pixel(this->r | other.r,
                         this->g | other.g,
                         this->b | other.b);
        }

        template <typename R> Pixel &operator +=(const Pixel<R> &other)
        {
            this->r += other.r;
            this->g += other.g;
            this->b += other.b;

            return *this;
        }

        template <typename R> Pixel &operator /=(const Pixel<R> &other)
        {
            this->r /= other.r;
            this->g /= other.g;
            this->b /= other.b;

            return *this;
        }

        Pixel &operator +=(QRgb pixel)
        {
            this->r += qRed(pixel);
            this->g += qGreen(pixel);
            this->b += qBlue(pixel);

            return *this;
        }

        T r;
        T g;
        T b;
};

typedef Pixel<qint8> PixelI8;
typedef Pixel<quint8> PixelU8;
typedef Pixel<qint32> PixelI32;
typedef Pixel<quint32> PixelU32;
typedef Pixel<qint64> PixelI64;
typedef Pixel<quint64> PixelU64;
typedef Pixel<qreal> PixelReal;

template<typename T> Pixel<T> exp(const Pixel<T> &pixel)
{
    return Pixel<T>(std::exp(pixel.r),
                    std::exp(pixel.g),
                    std::exp(pixel.b));
}

template <typename R, typename S> inline Pixel<R> mult(R c, const Pixel<S> &pixel)
{
    return Pixel<R>(c * pixel.r,
                    c * pixel.g,
                    c * pixel.b);
}

inline PixelU64 pow2(QRgb pixel)
{
    quint8 r = qRed(pixel);
    quint8 g = qGreen(pixel);
    quint8 b = qBlue(pixel);

    return PixelU64(r * r, g * g, b * b);
}

template<typename T> inline Pixel<T> pow2(const Pixel<T> &pixel)
{
    return Pixel<T>(pixel.r * pixel.r,
                    pixel.g * pixel.g,
                    pixel.b * pixel.b);
}

template<typename T> inline Pixel<T> sqrt(const Pixel<T> &pixel)
{
    return Pixel<T>(std::sqrt(pixel.r),
                    std::sqrt(pixel.g),
                    std::sqrt(pixel.b));
}

template<typename T> Pixel<T> bound(T min, const Pixel<T> &pixel, T max)
{
    return Pixel<T>(qBound(min, pixel.r, max),
                    qBound(min, pixel.g, max),
                    qBound(min, pixel.b, max));
}

template<typename T> inline Pixel<T> integralSum(const Pixel<T> *integral,
                                                 int lineWidth,
                                                 int x, int y, int kw, int kh)
{

    const Pixel<T> *p0 = integral + x + y * lineWidth;
    const Pixel<T> *p1 = p0 + kw;
    const Pixel<T> *p2 = p0 + kh * lineWidth;
    const Pixel<T> *p3 = p2 + kw;

    return *p0 + *p3 - *p1 - *p2;
}

template <typename T> inline Pixel<quint32> operator *(quint32 c, const Pixel<T> &pixel)
{
    return Pixel<quint32>(c * pixel.r,
                          c * pixel.g,
                          c * pixel.b);
}

void integralImage(const QImage &image,
                   QVector<PixelU8> &planes,
                   QVector<PixelU32> &integral,
                   QVector<PixelU64> &integral2)
{
    int oWidth = image.width() + 1;
    int oHeight = image.height() + 1;
    planes.resize(image.width() * image.height());
    integral.resize(oWidth * oHeight);
    integral2.resize(oWidth * oHeight);

    for (int y = 1; y < oHeight; y++) {
        const QRgb *line = (const QRgb *) image.constScanLine(y - 1);
        PixelU8 *planesLine = planes.data()
                              + (y - 1) * image.width();

        // Reset current line summation.
        PixelU32 sum;
        PixelU64 sum2;

        for (int x = 1; x < oWidth; x++) {
            QRgb pixel = line[x - 1];

            // Accumulate pixels in current line.
            sum += pixel;
            sum2 += pow2(pixel);

            // Offset to the current line.
            int offset = x + y * oWidth;

            // Offset to the previous line.
            // equivalent to x + (y - 1) * oWidth;
            int offsetPrevious = offset - oWidth;

            planesLine[x - 1] = pixel;

            // Accumulate current line and previous line.
            integral[offset] = sum + integral[offsetPrevious];
            integral2[offset] = sum2 + integral2[offsetPrevious];
        }
    }
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
    int mu = 0;
    qreal sigma = 1;

    // Add noise to the image
    qsrand(QTime::currentTime().msec());

    for (int i = 0; i < 100000; i++) {
        inImage.setPixel(qrand() % inImage.width(),
                         qrand() % inImage.height(),
                         qRgb(qrand() % 256,
                              qrand() % 256,
                              qrand() % 256));
    }

    int oWidth = inImage.width() + 1;
    QVector<PixelU8> planes;
    QVector<PixelU32> integral;
    QVector<PixelU64> integral2;
    integralImage(inImage, planes, integral, integral2);

    QElapsedTimer timer;
    timer.start();

    for (int y = 0, pos = 0; y < inImage.height(); y++) {
        const QRgb *iLine = (const QRgb *) inImage.constScanLine(y);
        QRgb *oLine = (QRgb *) outImage.scanLine(y);
        int yp = qMax(y - radius, 0);
        int kh = qMin(y + radius, inImage.height() - 1) - yp + 1;

        for (int x = 0; x < inImage.width(); x++, pos++) {
            int xp = qMax(x - radius, 0);
            int kw = qMin(x + radius, inImage.width() - 1) - xp + 1;

            // Calculate summation and cuadratic summation of the pixels.
            PixelU32 sum = integralSum(integral.constData(), oWidth,
                                       xp, yp, kw, kh);
            PixelU64 sum2 = integralSum(integral2.constData(), oWidth,
                                        xp, yp, kw, kh);
            qreal ks = kw * kh;

            // Calculate mean and standard deviation.
            PixelReal mean = sum / ks;
            PixelReal dev = sqrt(ks * sum2 - pow2(sum)) / ks;

            mean = bound(0., mean + mu, 255.);
            dev = bound(0., mult(sigma, dev), 127.);

            PixelReal sumP;
            PixelReal sumW;

            for (int j = 0; j < kh; j++) {
                const PixelU8 *line = planes.constData()
                                      + (yp + j) * inImage.width();

                for (int i = 0; i < kw; i++) {
                    // Calculate weighted avverage.
                    PixelU8 pixel = line[xp + i];
                    PixelReal d = mean - pixel;
                    PixelReal h = mult(-2., dev * dev);
                    PixelReal weight = exp(d * d / h);
                    sumP += weight * pixel;
                    sumW += weight;
                }
            }

            // Normalize result.
            sumP /= sumW;

            oLine[x] = qRgba(sumP.r, sumP.g, sumP.b, qAlpha(iLine[x]));
        }
    }

    qDebug() << timer.elapsed();
    outImage.save("mean.png");

    return EXIT_SUCCESS;
}
