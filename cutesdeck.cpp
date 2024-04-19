#include <QDebug>
#include <QPainter>
#include <QtConcurrent>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <linux/hidraw.h>
#include <sys/ioctl.h>

#include "cutesdeck.h"

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
    m_imgsize(72,72),
    m_autoOpen(true),
    m_background(Qt::black),
    m_foreground(Qt::yellow)
{
    memset(btnbuf, 0, 255);
    findInputDevices();
    enableUdevMonitoring();
}

void CuteSdeck::classBegin()
{

}

void CuteSdeck::componentComplete()
{
    if (m_devices.count()>0 && m_autoOpen) {
        openDeck(0);
    }
}

int CuteSdeck::findInputDevices()
{
    struct udev_enumerate *enumerate;
    struct udev_list_entry *devices, *dev_list_entry;
    struct hidraw_devinfo info;

    enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "hidraw");
    udev_enumerate_scan_devices(enumerate);
    devices = udev_enumerate_get_list_entry(enumerate);

    udev_list_entry_foreach(dev_list_entry, devices) {
        const char *path, *devpath, *pdevss, *product, *manufacturer, *vendor, *model, *serial;
        struct udev_device *dev;
        struct udev_device *pdev;
        int fd;

        path=udev_list_entry_get_name(dev_list_entry);
        dev=udev_device_new_from_syspath(udev, path);
        devpath=udev_device_get_devnode(dev);

        if (!devpath)
            continue;

        if (!probeDevice(devpath))
            continue;

        // Get parent USB device
        pdev=udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_device");

        product=udev_device_get_property_value(pdev, "ID_MODEL");
        manufacturer=udev_device_get_property_value(pdev, "ID_VENDOR");
        vendor=udev_device_get_property_value(pdev, "ID_VENDOR_ID");
        model=udev_device_get_property_value(pdev, "ID_MODEL_ID");
        serial=udev_device_get_property_value(pdev, "ID_SERIAL");

        m_serial=serial;
        m_devices.insert(devpath, serial);

        udev_device_unref(dev);
    }

    qDebug() << "Found devices: "  << m_devices;

    emit devicesChanged();

    return m_devices.count();
}

short CuteSdeck::getVendorProduct(int fd, struct hidraw_devinfo &info)
{
    int res = ioctl(fd, HIDIOCGRAWINFO, &info);
    if (res < 0)
        return -1;

    return info.product;
}

bool CuteSdeck::probeDevice(const char *devpath)
{
    int fd;
    struct hidraw_devinfo info;

    qDebug() << "Checking device: " << devpath;

    fd=open(devpath, O_RDONLY);
    if (fd<0) {
        perror("Open failed");
        qDebug() << "Failed to open device for probe " << devpath;
        return false;
    }

    int product=getVendorProduct(fd, info);
    if (product < 0) {
        close(fd);
        return false;
    }

    close(fd);

    if (info.vendor==VENDOR_ELGATO) {
        printf("Found product: 0x%04hx\n", info.product);
        return true;
    }

    return false;
}

void CuteSdeck::readUdevMonitor()
{
    struct udev_device *dev;

    dev = udev_monitor_receive_device(mon);
    if (!dev) {
        qWarning("No Device from receive_device(). An error occured.");
        return;
    }
    const char *action=udev_device_get_action(dev);
    const char *devpath=udev_device_get_devnode(dev);

    qDebug() << "uDev: " << action << devpath;

    if (strcmp(action, "add")==0 && devpath) {
        qDebug() << "New device, checking: " << devpath;
        if (probeDevice(devpath)) {
            struct udev_device *pdev;
            const char *serial;

            pdev=udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_device");
            serial=udev_device_get_property_value(pdev, "ID_SERIAL");

            m_devices.insert(devpath, serial);
        }
    } else if (strcmp(action, "remove")==0 && devpath) {
        int r=m_devices.remove(devpath);
    }

    qDebug() << "Found devices: "  << m_devices;    

    udev_device_unref(dev);

    emit devicesChanged();
}

void CuteSdeck::enableUdevMonitoring() {
    mon = udev_monitor_new_from_netlink(udev, "udev");
    udev_monitor_filter_add_match_subsystem_devtype(mon, "hidraw", NULL);
    udev_monitor_enable_receiving(mon);
    udev_fd = udev_monitor_get_fd(mon);
    m_udev_notifier = new QSocketNotifier(udev_fd, QSocketNotifier::Read, this);
    connect(m_udev_notifier, SIGNAL(activated(int)), this, SLOT(readUdevMonitor()));
}

CuteSdeck::~CuteSdeck()
{
    closeDeck();
}

bool CuteSdeck::openDeck(int id)
{
    if (hid_fd>0) {
        qWarning("Device already open");
        return false;
    }

    if (m_devices.empty()) {
        qWarning("No devices");
        return false;
    }

    const char *tmp = m_devices.firstKey().toLocal8Bit().data();

    hid_fd = open(tmp, O_RDWR | O_NONBLOCK);

    struct hidraw_devinfo info;
    int product=getVendorProduct(hid_fd, info);

    if (!hid_fd) {
        qWarning("Device not found");
        return false;
    }

    switch (product) {
    case DeckUnknown:
        qWarning("Unknown?");
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

    getBrightness();

    m_hid_notifier = new QSocketNotifier(hid_fd, QSocketNotifier::Read, this);
    connect(m_hid_notifier, SIGNAL(activated(int)), this, SLOT(readDeck()));

    return true;
}

bool CuteSdeck::closeDeck()
{
    if (m_hid_notifier) {
        m_hid_notifier->disconnect();
        delete m_hid_notifier;
        m_hid_notifier=nullptr;
    }

    if (hid_fd>0) {
        close(hid_fd);
        hid_fd=-1;
    }

    emit isOpenChanged();

    return true;
}

void CuteSdeck::readDeck()
{
    unsigned char buf[255];
    int res;

    if (hid_fd<0)
        return;    

    res=read(hid_fd, buf, 64);

    if (res==-1) {
        qWarning("read -1");
        closeDeck();
        emit error();
        return;
    }

    if (res==0) {
        qWarning("read 0");
        return;
    }

    for (int i=0;i<m_buttons;i++) {
        if (buf[KEY_OFFSET+i]==1) {
            emit keyPressed(i);
        } else if (buf[KEY_OFFSET+i]!=btnbuf[i]) {
            emit keyReleased(i);
        }
        btnbuf[i]=buf[KEY_OFFSET+i];
    }
}

int CuteSdeck::setImagePart(char key, char part)
{
    if (hid_fd<0)
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

    return write(hid_fd, img_buffer, 8191);
}

int CuteSdeck::resetImages()
{
    if (hid_fd<0)
        return -1;

    memset(img_buffer, 0, 8191);
    img_buffer[0]=0x02;

    return write(hid_fd, img_buffer, 1024);
}

int CuteSdeck::resetDeck()
{
    int r;

    memset(img_buffer, 0, 32);
    img_buffer[0]=0x03;
    img_buffer[1]=0x02;

    r=hidraw_send_feature_report(img_buffer, 32);
    if (r==-1)
        qWarning("resetDeck failed");
    return r;
}

int CuteSdeck::setBrightness(uint8_t percent)
{
    int r;    

    memset(img_buffer, 0, 32);
    img_buffer[0]=0x03;
    img_buffer[1]=0x08;
    img_buffer[2]=percent;

    r=hidraw_send_feature_report(img_buffer, 32);
    if (r==-1)
        qWarning("setBrightness failed");
    return r;
}

int CuteSdeck::getBrightness()
{
    return -1;
}


bool CuteSdeck::setImageJPG(uint8_t key, const QString file)
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

bool CuteSdeck::setImageText(uint8_t key, const QString txt)
{
    QImage img=QImage(m_imgsize, QImage::Format_RGB32);
    img.fill(m_background);

    QPainter p;
    if (!p.begin(&img)) return false;

    p.setPen(QPen(m_foreground));
    p.setFont(QFont("Times", 12, QFont::Bold));
    p.drawText(img.rect(), Qt::AlignCenter, txt);
    p.end();

    img.mirror(true, true);

    return setImage(key, img);
}

bool CuteSdeck::setImage(uint8_t key, const QImage &img, bool scale)
{
    QByteArray tmp;
    QBuffer buf(&tmp);
    QImage imgc=img;

    if (hid_fd<0)
        return false;

    if (img.isNull())
        return false;

    if (m_imgsize.width()==0 || m_imgsize.height()==0)
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

bool CuteSdeck::setImage(uint8_t key, const char *img, ssize_t imgsize)
{
    int pn=0,len,sent,r=0;
    ssize_t remain;

    if (hid_fd<0)
        return -1;

    if (key>m_buttons) {
        return -1;
    }

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

        r=write(hid_fd, img_buffer, 1024);
        if (r<0)
            break;

        remain=remain-len;
        pn++;
    }

    return r>-1;
}

QString CuteSdeck::serialNumber()
{
    if (hid_fd<0)
        return NULL;

    return m_serial;
}

bool CuteSdeck::isOpen()
{
    return hid_fd>0;
}

QString CuteSdeck::serial() const
{
    return m_serial;
}

uint CuteSdeck::buttons() const
{
    return m_buttons;
}

int CuteSdeck::hidraw_send_feature_report(const unsigned char *data, size_t length)
{
    if (hid_fd<0)
        return -1;

    return ioctl(hid_fd, HIDIOCSFEATURE(length), data);
}

int CuteSdeck::hidraw_get_feature_report(const unsigned char *data, size_t length)
{
    if (hid_fd<0)
        return -1;

    return ioctl(hid_fd, HIDIOCGFEATURE(length), data);
}

bool CuteSdeck::autoOpen() const
{
    return m_autoOpen;
}

void CuteSdeck::setAutoOpen(bool newAutoOpen)
{
    if (m_autoOpen == newAutoOpen)
        return;
    m_autoOpen = newAutoOpen;
    emit autoOpenChanged();
}

uint CuteSdeck::devices() const
{
    return m_devices.count();
}

QColor CuteSdeck::background() const
{
    return m_background;
}

void CuteSdeck::setBackground(const QColor &newBackground)
{
    if (m_background == newBackground)
        return;
    m_background = newBackground;
    emit backgroundChanged();
}

QColor CuteSdeck::foreground() const
{
    return m_foreground;
}

void CuteSdeck::setForeground(const QColor &newForeground)
{
    if (m_foreground == newForeground)
        return;
    m_foreground = newForeground;
    emit foregroundChanged();
}
