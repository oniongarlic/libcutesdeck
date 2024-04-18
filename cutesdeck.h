#ifndef CUTESDECK_H
#define CUTESDECK_H

#include "qsocketnotifier.h"
#include <QObject>
#include <QImage>
#include <QFuture>
#include <QQmlEngine>
#include <QQmlParserStatus>

#include <libudev.h>
#include <linux/hidraw.h>

class CuteSdeck : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    QML_NAMED_ELEMENT(CuteStreamDeck)
    Q_PROPERTY(bool isOpen READ isOpen NOTIFY isOpenChanged FINAL)
    Q_PROPERTY(bool autoOpen READ autoOpen WRITE setAutoOpen NOTIFY autoOpenChanged FINAL)
    Q_PROPERTY(QString serial READ serial NOTIFY serialChanged FINAL)
    Q_PROPERTY(uint devices READ devices NOTIFY devicesChanged FINAL)
    Q_PROPERTY(uint buttons READ buttons NOTIFY buttonsChanged FINAL)

public:
    explicit CuteSdeck(QObject *parent = nullptr);

    void classBegin() override;
    void componentComplete() override;

    enum Devices {
        DeckUnknown=0,
        DeckOriginal=0x0060,
        DeckOriginalV2=0x006d,
        DeckXL=0x006c,
        DeckXLV2=0x008f,
        DeckMK2=0x0080,
        DeckPedal=0x0086,
        DeckMiniMK2=0x0090
    };
    Q_ENUM(Devices)

    ~CuteSdeck();
    bool isOpen();

    QString serial() const;

    uint buttons() const;

    bool autoOpen() const;

    void setAutoOpen(bool newAutoOpen);

    int getBrightness();
    uint devices() const;

public slots:
    bool openDeck(int id);
    bool closeDeck();

    QString serialNumber();

    int setBrightness(uint8_t percent);
    int resetDeck();

    bool setImage(uint8_t key, const char *img, ssize_t imgsize);
    bool setImage(uint8_t key, const QImage &img, bool scale=false);
    int resetImages();

    bool setImageText(uint8_t key, const QString txt);
    bool setImageJPG(uint8_t key, const QString file);
signals:
    void keyPressed(quint8 key);
    void error();

    void isOpenChanged();
    void serialChanged();
    void buttonsChanged();
    void autoOpenChanged();
    void devicesChanged();

protected:
    int setImagePart(char key, char part);

private slots:
    void readDeck();
    void readUdevMonitor();
private:
    QString m_serial;
    QSize m_imgsize;
    uint8_t m_buttons=0;
    bool m_autoOpen;

    bool have_udev;
    struct udev *udev;
    struct udev_monitor *mon;
    struct libevdev *edev;
    int udev_fd=-1;
    int hid_fd=-1;

    QSocketNotifier *m_hid_notifier;
    QSocketNotifier *m_udev_notifier;    
    QMap<QString, QString> m_devices;

    int findInputDevices();
    int hidraw_send_feature_report(const unsigned char *data, size_t length);
    int hidraw_get_feature_report(const unsigned char *data, size_t length);
    void enableUdevMonitoring();
    bool probeDevice(const char *devpath);
    short getVendorProduct(int fd, hidraw_devinfo &info);

};

#endif // CUTESDECK_H
