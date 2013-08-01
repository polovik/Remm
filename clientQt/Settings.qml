import QtQuick 2.1
import QtQuick.Controls 1.0

Item {
    width: 600
    height: 600
    signal connectionEstablished()
    Rectangle {
        id: rectangle1
        x: 0
        y: 0
        color: "yellowgreen"
        anchors.rightMargin: 0
        anchors.bottomMargin: 0
        anchors.leftMargin: 0
        anchors.topMargin: 0
        anchors.fill: parent

        ExclusiveGroup { id: groupConnectionType }

        RadioButton {
            id: radio_button_IPv6
            anchors.top: parent.top
            anchors.topMargin: (parent.height - height) / 3
            anchors.left: parent.left
            anchors.leftMargin: (parent.width - width - text_IPv6_RPi_URL.width) / 3
            text: "IPv6(conection over 3G Internet)"
            exclusiveGroup: groupConnectionType
            onCheckedChanged: {
                if (checked)
                    text_IPv6_RPi_URL.enabled = true
                else
                    text_IPv6_RPi_URL.enabled = false
            }
        }

        TextField {
            id: text_IPv6_RPi_URL
            width: 209
            height: 25
            anchors.top: radio_button_IPv6.top
            anchors.left: radio_button_IPv6.right
            anchors.leftMargin: (parent.width - width - radio_button_IPv6.width) / 3
            text: "remm.broker.freenet6.net"
            enabled: false
            visible: true
//                anchors.top: parent.top
//                anchors.topMargin: (parent.height - height) / 2
//                anchors.left: parent.left
//                anchors.leftMargin: (parent.width - width) / 2
            placeholderText: ""
        }

        RadioButton {
            id: radio_button_IPv4
            anchors.top: radio_button_IPv6.bottom
            anchors.topMargin: 50
            anchors.left: radio_button_IPv6.left
            text: "IPv4(direct Ethernet connection)"
            checked: true
            exclusiveGroup: groupConnectionType
            onCheckedChanged: {
                if (checked)
                    text_IPv4_RPi_IP.enabled = true
                else
                    text_IPv4_RPi_IP.enabled = false
            }
        }

        TextField {
            id: text_IPv4_RPi_IP
            width: 209
            height: 25
            anchors.top: radio_button_IPv4.top
            anchors.left: text_IPv6_RPi_URL.left
            text: "192.168.1.100"
            placeholderText: ""
        }

        Label {
            id: label_Port
            anchors.top: parent.top
            anchors.topMargin: (parent.height - height) / 3 * 2
            anchors.left: buttonConnectRPi.left
            text: "Port:"
        }

        TextField {
            id: textRPiPort
            width: 68
            height: 25
            anchors.verticalCenter: label_Port.verticalCenter
            anchors.right: buttonConnectRPi.right
            text: "9512"
            readOnly: true
            placeholderText: "Text Field"
        }

        Button {
            id: buttonConnectRPi
            width: 117
            height: 32
            anchors.top: parent.top
            anchors.topMargin: (parent.height - height) / 6 * 5
            anchors.left: parent.left
            anchors.leftMargin: (parent.width - width) / 2
            text: "Connect"
            checkable: false
            checked: false
            onClicked: {
                connectionEstablished()
                if (radio_button_IPv6.checked)
                    connectRPi.tryDirectConnectToRPi(text_IPv6_RPi_URL.text, Number(textRPiPort.text))
                else
                    connectRPi.tryDirectConnectToRPi(text_IPv4_RPi_IP.text, Number(textRPiPort.text))
            }
        }

    }
}
