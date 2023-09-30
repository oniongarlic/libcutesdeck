#ifndef CUTESDECK_H
#define CUTESDECK_H

#include <QObject>
#include <QImage>
#include <QFuture>
#include <QQmlEngine>

#include <hidapi/hidapi.h>
#include <linux/input.h>
// #include <libevdev/libevdev-uinput.h>

// #include "libcutesdeck_global.h"

class CuteSdeck : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool isOpen READ isOpen NOTIFY isOpenChanged FINAL)
    Q_PROPERTY(QString serial READ serial NOTIFY serialChanged FINAL)

    Q_PROPERTY(uint buttons READ buttons NOTIFY buttonsChanged FINAL)

public:
    explicit CuteSdeck(QObject *parent = nullptr);

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

public slots:
    bool open(Devices id);
    bool open(Devices id, QString serial);
    bool close();

    void start();

    QString serialNumber();

    int setBrightness(uint8_t percent);
    int resetDeck();

    bool setImage(uint8_t key, const char *img, ssize_t imgsize);
    bool setImage(uint8_t key, const QImage &img, bool scale=false);
    int resetImages();

    bool setImageText(uint8_t key, const QString txt);
    bool setImageJPG(uint8_t key, const QString file);
signals:
    void keyPressed(uint8_t key);
    void isOpenChanged();
    void error();

    void serialChanged();

    void buttonsChanged();

protected:
    int setImagePart(char key, char part);

private:
    QFuture<void> m_future;
    QMutex mutex;
    bool m_running=false;
    hid_device *handle=nullptr;
    QString m_serial;
    //struct libevdev *dev;
    //struct libevdev_uinput *uidev;
    QSize m_imgsize;
    uint8_t m_buttons=0;
    void loop();
};

#endif // CUTESDECK_H
