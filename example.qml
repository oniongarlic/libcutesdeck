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

    property int page: 0

    ListModel {
        id: pages
        ListElement {
            page: 0
            buttons: [
                ListElement { button: 0; title: "Cut" },
                ListElement { button: 1; title: "Copy" },
                ListElement { button: 2; title: "Paste" },
                ListElement { button: 3; title: "Undo" },
                ListElement { button: 4; title: "Redo" },

                ListElement { button: 5; title: "Insert" },
                ListElement { button: 6; title: "Replce" },
                ListElement { button: 7; title: "Cover" },
                ListElement { button: 8; title: "Append" },
                ListElement { button: 9; title: "Prepend" },

                ListElement { button: 11; title: "1" },
                ListElement { button: 12; title: "2" },
                ListElement { button: 13; title: "3" }
            ]
        }
        ListElement {
            page: 1
            buttons: [
                ListElement { button: 0; title: "CAM 1" },
                ListElement { button: 1; title: "CAM 2" },
                ListElement { button: 2; title: "CAM 3" },
                ListElement { button: 3; title: "CAM 4" },
                ListElement { button: 4; title: "CAM 5" },

                ListElement { button: 5; title: "M1" },
                ListElement { button: 6; title: "M2" },
                ListElement { button: 7; title: "M3" },
                ListElement { button: 8; title: "M4" },
                ListElement { button: 9; title: "M5" },

                ListElement { button: 11; title: "Play" },
                ListElement { button: 12; title: "Stop" },
                ListElement { button: 13; title: "Record" }
            ]
        }
    }

    function setButtonPage(pi) {
        console.debug("pages", pages.count)
        if (pi>pages.count-1)
            pi=pages.count-1;
        if (pi<0)
            pi=0;
        console.debug("pi", pi)
        let p=pages.get(pi)
        let b=p.buttons

        for (let i=0; i<b.count ;i++) {
            csd.setImageText(i, b.get(i).title);
        }

        return pi;
    }

    header: ToolBar {
        RowLayout {
            ToolButton {
                text: "Open"
                enabled: !csd.isOpen && csd.devices>0
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
                enabled: csd.isOpen
                text: "Set text"
                onClicked: {
                    csd.setImageText(0, "CAM\n1");
                    csd.setImageText(1, "CAM\n2");
                    csd.setImageText(2, "CAM\n3");
                    csd.setImageText(3, "CAM\n4");
                    csd.setImageText(4, "Super\nSource");

                    csd.setImageText(5, "CAM\n5");
                    csd.setImageText(6, "CAM\n6");
                    csd.setImageText(7, "CAM\n7");
                    csd.setImageText(8, "CAM\n8");
                    csd.setImageText(9, "M/E\n2");
                }
            }
            ToolButton {
                enabled: csd.isOpen
                text: "Set images"
                onClicked: {
                    csd.setImageJPG(0, "icons/play.jpg")
                    csd.setImageJPG(1, "icons/play.jpg")
                    csd.setImageJPG(2, "icons/play.jpg")
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
            Label {
                id: btnText
            }
        }
    }

    Component {
        id: deckButton
        Rectangle {
            id: dbc
            Layout.fillHeight: true
            Layout.fillWidth: true
            border.width: 2
            color: csd.buttonPressed==index ? "yellow" : "black"
            required property int index
            Text {
                id: btn
                color: "#14e83b"
                anchors.centerIn: parent
                font.pixelSize: 64
                text: dbc.index
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 16
        color: "grey"
        GridLayout {
            anchors.fill: parent
            rows: 3
            columns: 5
            rowSpacing: 4
            columnSpacing: 4
            Repeater {
                model: 15//csd.buttons
                delegate: deckButton
            }
        }
    }

    CuteStreamDeck {
        id: csd
        autoOpen: true
        background: "black"
        foreground: "green"

        property int buttonPressed: -1

        onIsOpenChanged: {
            console.debug("onIsOpenChanged", isOpen);
            if (isOpen) {
                console.debug(serialNumber())
                csd.resetDeck();
                csd.setImageText(0, "Hello");
                csd.setImageText(1, "World");

                csd.setImageText(13, "Prev");
                csd.setImageText(14, "Next");
            }
        }

        onButtonsChanged: {
            console.debug("Buttons count", buttons)
        }

        onSerialChanged: {
            console.debug("New serial", serial)
        }

        onKeyPressed: (key) => {
                          buttonPressed=key;
                          console.debug(key)
                          btnText.text=key
                          switch (key) {
                              case 13:
                              page=setButtonPage(--page);
                              break;
                              case 14:
                              page=setButtonPage(++page);
                              break;
                              case 1:
                              page=0;
                              break;
                          }
                      }
        onKeyReleased: {
            buttonPressed=-1;
        }
    }

}
