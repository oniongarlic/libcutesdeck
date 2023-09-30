#include <QDebug>
#include <QPainter>

#include "cutesdeck.h"

#include <QtConcurrent>

#define MAX_STR 255

#define POS_SE 2 // PACKET SEQUENCE NUMBER
#define POS_PR 4 // PREVOUS PACKET SEQUENCE NUMBER
#define POS_KI 5 // KEY INDEX

#define IMAGE_1 7749
#define IMAGE_2 7803

#define KEY_OFFSET 4

//                       SE    PR KI
static uint8_t img_header[]={ 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static uint8_t img_extra_v1[]={ 0x42, 0x4d, 0xf6, 0x3c, 0, 0, 0, 0, 0, 0, 0x36, 0, 0, 0, 0x28, 0, 0, 0, 0x48, 0, 0, 0, 0x48, 0, 0, 0, 0x01, 0, 0x18, 0, 0, 0, 0, 0, 0xc0, 0x3c, 0, 0, 0x13, 0x0e, 0, 0, 0x13, 0x0e, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static uint8_t img_buffer[8191];
static uint8_t img_data[65535];

#define VENDOR_ELGATO 0x0fd9

CuteSdeck::CuteSdeck(QObject *parent)
    : QObject{parent},
    m_imgsize(72,72)
{
    int res = hid_init();
    if (res!=0) {
        qWarning("HID Init failed");
    }
}

CuteSdeck::~CuteSdeck()
{
    close();
}

bool CuteSdeck::open(Devices id)
{
    QString dummy;

    return open(id, dummy);
}

bool CuteSdeck::open(Devices id, QString serial)
{
    if (handle) {
        qWarning("Device already open");
        return false;
    }
    if (serial.isEmpty()) {
        handle = hid_open(VENDOR_ELGATO, id, NULL);
    } else {
        std::wstring tmp=serial.toStdWString();
        handle = hid_open(VENDOR_ELGATO, id, tmp.c_str());
    }
    if (!handle) {
        qWarning("Device not found");
        return false;
    }
    switch (id) {

    case DeckUnknown:
    case DeckOriginal:
        qWarning("Untested");
    case DeckOriginalV2:
        m_imgsize.setHeight(72);
        m_imgsize.setWidth(72);
        m_buttons=15;
        break;
    case DeckXL:
    case DeckXLV2:
        qWarning("Untested");
        m_imgsize.setHeight(96);
        m_imgsize.setWidth(96);
        m_buttons=32;
        break;
    case DeckMK2:
        qWarning("Untested");
        m_imgsize.setHeight(72);
        m_imgsize.setWidth(72);
        m_buttons=15;
    case DeckMiniMK2:
        qWarning("Untested");
        m_imgsize.setHeight(72);
        m_imgsize.setWidth(72);
        m_buttons=6;
        break;
    case DeckPedal:
        qWarning("Untested");
        m_imgsize.setHeight(0);
        m_imgsize.setWidth(0);
        m_buttons=3;
        break;
    }

    emit isOpenChanged();

    m_serial=serialNumber();
    emit serialChanged();

    qDebug() << "Button image size is " << m_imgsize;
    qDebug() << "Buttons " << m_buttons;

    emit buttonsChanged();

    return true;
}

bool CuteSdeck::close()
{
    m_running=false;

    m_future.waitForFinished();

    if (handle)
        hid_close(handle);

    handle=nullptr;

    emit isOpenChanged();

    return true;
}

void CuteSdeck::start()
{
    if (m_running)
        return;
    m_running = true;
    m_future = QtConcurrent::run(&CuteSdeck::loop, this);    
}

void CuteSdeck::loop()
{
    unsigned char buf[255];
    int res;

    if (!handle)
        return;

    qDebug("Listening to buttons.");

    QMutexLocker locker(&mutex);

    while (m_running) {
        if (!locker.isLocked())
            locker.relock();
        res=hid_read_timeout(handle, buf, 64, 100);
        locker.unlock();

        if (res==-1) {
            qWarning("hid_read_timeout error");
            m_running = false;
            emit error();
            return;
        }

        if (res==0) {            
            QThread::msleep(20);
            continue;
        }

        for (int i=0;i<m_buttons;i++) {
            if (buf[KEY_OFFSET+i]==1) {
                emit keyPressed(i);
            }
        }
    }
    qDebug("Stopped listening to buttons.");
}

int CuteSdeck::setImagePart(char key, char part)
{
    if (!handle)
        return -1;

    memset(img_buffer, 0, 8191);
    img_header[POS_SE]=part;
    img_header[POS_PR]=part==1 ? 0 : 1;
    img_header[POS_KI]=key+1;
    memcpy(img_buffer, img_header, sizeof(img_header));
    if (part==1) {
        memcpy(img_buffer+sizeof(img_header), img_extra_v1, sizeof(img_extra_v1));
    } else {
        // memcpy(img_buffer+sizeof(img_header), img_extra, sizeof(img_extra));
    }

    return hid_write(handle, img_buffer, 8191);
}

int CuteSdeck::resetImages()
{
    if (!handle)
        return -1;

    memset(img_buffer, 0, 8191);
    img_buffer[0]=0x02;

    QMutexLocker locker(&mutex);

    return hid_write(handle, img_buffer, 1024);
}

int CuteSdeck::resetDeck()
{
    int r;

    if (!handle)
        return -1;

    memset(img_buffer, 0, 32);
    img_buffer[0]=0x03;
    img_buffer[1]=0x02;

    QMutexLocker locker(&mutex);

    r=hid_send_feature_report(handle, img_buffer, 32);
    if (r==-1)
        qWarning("resetDeck failed");
    return r;
}

int CuteSdeck::setBrightness(char percent)
{
    int r;

    if (!handle)
        return -1;

    memset(img_buffer, 0, 32);
    img_buffer[0]=0x03;
    img_buffer[1]=0x08;
    img_buffer[2]=percent;

    QMutexLocker locker(&mutex);

    r=hid_send_feature_report(handle, img_buffer, 32);
    if (r==-1)
        qWarning("setBrightness failed");
    return r;
}

bool CuteSdeck::setImageJPG(char key, const QString file)
{
    QByteArray data;
    QFile f(file);

    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to load file " << file << f.errorString();
        return false;
    }

    data=f.readAll();
    f.close();

    return setImage(key, data.data(), data.size());
}

bool CuteSdeck::setImageText(char key, const QString txt)
{
    QImage img=QImage(m_imgsize, QImage::Format_RGB32);
    img.fill(0);

    QPainter p;
    if (!p.begin(&img)) return false;

    p.setPen(QPen(Qt::yellow));
    p.setFont(QFont("Times", 12, QFont::Bold));
    p.drawText(img.rect(), Qt::AlignCenter, txt);
    p.end();

    img.mirror(true, true);

    return setImage(key, img);
}

bool CuteSdeck::setImage(char key, const QImage &img, bool scale)
{
    QByteArray tmp;
    QBuffer buf(&tmp);
    QImage imgc=img;

    if (!handle)
        return false;

    if (img.isNull())
        return false;

    if (!scale && img.width()!=m_imgsize.width() && img.height()!=m_imgsize.height()) {
        qWarning() << "Button image must be " << m_imgsize;
        return false;
    } else if (scale) {
        imgc=img.scaled(m_imgsize, Qt::IgnoreAspectRatio);
    }

    buf.open(QIODevice::WriteOnly);
    if (imgc.save(&buf, "jpg", 100)) {
        setImage(key, tmp.data(), (ssize_t)tmp.size());
        return true;
    }
    return false;
}

bool CuteSdeck::setImage(char key, const char *img, ssize_t imgsize)
{
    int pn=0,len,sent,r=0;
    ssize_t remain;

    if (!handle)
        return -1;

    QMutexLocker locker(&mutex);

    pn=0;
    remain=imgsize;

    while (remain>0) {
        len=remain<1016 ? remain : 1016;
        sent=pn*1016;

        img_header[0]=0x02;
        img_header[1]=0x07;
        img_header[2]=key;

        img_header[3]=len==remain ? 1 : 0;

        img_header[4]=len & 0xff;
        img_header[5]=len >> 8;

        img_header[6]=pn & 0xff;
        img_header[7]=pn >> 8;

        memset(img_buffer, 0, 1024);
        memcpy(img_buffer, img_header, 8);
        memcpy(img_buffer+8, img+sent, len);

        r=hid_write(handle, img_buffer, 1024);
        if (r<0)
            break;

        remain=remain-len;
        pn++;
    }

    return r>-1;
}

QString CuteSdeck::serialNumber()
{
    wchar_t buf[50];

    if (!handle)
        return NULL;

    QMutexLocker locker(&mutex);

    hid_get_serial_number_string(handle, buf, 50);
    return QString::fromWCharArray(buf);
}

bool CuteSdeck::isOpen()
{
    QMutexLocker locker(&mutex);

    return handle!=nullptr;
}

QString CuteSdeck::serial() const
{
    return m_serial;
}

uint CuteSdeck::buttons() const
{
    return m_buttons;
}
