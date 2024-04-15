import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts

//import org.tal

import CutesDeck

ApplicationWindow {
    width: 800
    height: 520
    minimumHeight: 540
    minimumWidth: 640
    visible: true
    title: qsTr("CuteStreamDeck")

    header: ToolBar {
        RowLayout {
            ToolButton {
                text: "Open"
                enabled: !csd.isOpen
                onClicked: {
                    csd.openDeck(CuteStreamDeck.DeckOriginalV2)
                }
            }
            ToolButton {
                text: "Close"
                enabled: csd.isOpen
                onClicked: {
                    csd.closeDeck()
                }
            }
            ToolButton {
                enabled: csd.isOpen
                text: "Reset deck"
                onClicked: csd.resetDeck();
            }
            ToolButton {
                enabled: csd.isOpen
                text: "Reset images"
                onClicked: csd.resetImages()
            }
            ToolButton {
                text: "Set text"
                onClicked: {
                    csd.setImageText(0, "Record");
                    csd.setImageText(1, "Play");
                    csd.setImageText(2, "Stop");
                    csd.setImageText(3, "Cut");
                    csd.setImageText(4, "Auto");

                    csd.setImageText(5, "CAM\n1");
                    csd.setImageText(6, "CAM\n2");
                    csd.setImageText(7, "CAM\n3");
                    csd.setImageText(8, "CAM\n4");
                    csd.setImageText(9, "Super\nSource");

                    csd.setImageText(10, "CAM\n5");
                    csd.setImageText(11, "CAM\n6");
                    csd.setImageText(12, "CAM\n7");
                    csd.setImageText(13, "CAM\n8");

                    csd.setImageJPG(14, ":/button-0.jpg")
                }
            }
            Slider {
                from: 0
                to: 100
                onValueChanged: csd.setBrightness(value)
            }
        }
    }

    footer: ToolBar {
        RowLayout {
            Label {
                id: serialText
                text: csd.serial
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "red"
        Text {
            id: btnText
            anchors.centerIn: parent
            font.pixelSize: 64
        }
    }

    CuteStreamDeck {
        id: csd

        onIsOpenChanged: {
            if (isOpen)
                console.debug(serialNumber())
        }

        onKeyPressed: (key) => {
            console.debug(key)
            btnText.text=key
        }
    }

}
