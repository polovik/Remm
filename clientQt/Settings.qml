import QtQuick 2.1
import QtQuick.Controls 1.0

Item {
    width: 600
    height: 600
    signal connectionEstablished()
    Rectangle {
        id: rectangle1
        color: "yellowgreen"
        anchors.fill: parent

        TextField {
            id: textRPiURL
            width: 209
            height: 25
            text: "remm.broker.freenet6.net"
            anchors.top: parent.top
            anchors.topMargin: (parent.height - height) / 2
            anchors.left: parent.left
            anchors.leftMargin: (parent.width - width) / 2
            placeholderText: "Text Field"
        }

        TextField {
            id: textRPiPort
            width: 68
            height: 25
            text: "9512"
            anchors.top: parent.top
            anchors.topMargin: (parent.height - height) / 2
            anchors.left: parent.left
            anchors.leftMargin: textRPiURL.x + textRPiURL.width + 50
            placeholderText: "Text Field"
        }

        Button {
            id: buttonConnectRPi
            width: 117
            height: 32
            text: "Connect"
            anchors.top: textRPiPort.bottom
            anchors.topMargin: 20
            anchors.right: textRPiPort.right
            checkable: false
            checked: false
            onClicked: {
                connectionEstablished()
                connectRPi.tryDirectConnectToRPi(textRPiURL.text, Number(textRPiPort.text))
            }
        }
    }
}
