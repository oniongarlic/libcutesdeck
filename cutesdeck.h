#ifndef CUTESDECK_H
#define CUTESDECK_H

#include <QObject>
#include <QImage>
#include <QFuture>

#include <hidapi.h>
#include <linux/input.h>
// #include <libevdev/libevdev-uinput.h>

// #include "libcutesdeck_global.h"

class CuteSdeck : public QObject
{
    Q_OBJECT
    Q_ENUMS(Devices)
    Q_PROPERTY(bool isOpen READ isOpen NOTIFY isOpenChanged FINAL)
    Q_PROPERTY(QString serial READ serial NOTIFY serialChanged FINAL)

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

    ~CuteSdeck();
    bool isOpen();

    QString serial() const;

public slots:
    bool open(Devices id);
    bool open(Devices id, QString serial);
    bool close();

    void start();

    QString serialNumber();

    int setBrightness(char percent);
    int resetDeck();

    bool setImage(char key, const char *img, ssize_t imgsize);
    bool setImage(char key, const QImage &img, bool scale=false);
    int resetImages();

    bool setImageText(char key, const QString txt);
    bool setImageJPG(char key, const QString file);
signals:
    void keyPressed(int key);
    void isOpenChanged();
    void error();

    void serialChanged();

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
    int m_buttons=0;
    void loop();
};

#endif // CUTESDECK_H
