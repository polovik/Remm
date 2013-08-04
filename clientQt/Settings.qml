import QtQuick 2.1
import QtQuick.Controls 1.0

Item {
    width: 600
    height: 600
    signal connectionEstablished()

    Rectangle {
        id: rectangle_address
        color: "yellowgreen"
        x: 0
        y: 0
        height: parent.height / 3
        width: parent.width

        ExclusiveGroup { id: groupConnectionType }

        RadioButton {
            id: radio_button_IPv6
            anchors.top: parent.top
            anchors.topMargin: parent.height / 5
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
            placeholderText: ""
        }

        RadioButton {
            id: radio_button_IPv4
            anchors.top: radio_button_IPv6.bottom
            anchors.topMargin: 30
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
            x: (parent.width - width - textRPiPort.width - 20) / 2
            y: (parent.height - height) / 5 * 4
            text: "Port:"
        }

        TextField {
            id: textRPiPort
            width: 68
            height: 25
            x: label_Port.x + label_Port.width + 20
            anchors.verticalCenter: label_Port.verticalCenter
            text: "9512"
            readOnly: true
            placeholderText: "Text Field"
        }

        Label {
            id: label_address
            x: -30
            anchors.verticalCenter: parent.verticalCenter
            text: "Address"
            font.bold: true
            font.family: "Courier New"
            font.pointSize: 21
            rotation: -90
        }

    }

    Rectangle {
        id: rectangle_camera
        color: "#e49023"
        anchors.top: rectangle_address.bottom
        height: parent.height / 3
        width: parent.width

        Label {
            id: label_resolution
            x: 63
            y: (parent.height - combo_resolution.height - height) / 4
            text: "Resolution:"
        }
        ComboBox {
            id: combo_resolution
            anchors.left: label_resolution.left
            y: (parent.height - label_resolution.height - height) / 2
            model: ListModel {
                id: modes
                ListElement { text: "960x720"; horRes: 960; verRes: 720 }
                ListElement { text: "640x480"; horRes: 640; verRes: 480 }
                ListElement { text: "320x240"; horRes: 320; verRes: 240 }
                ListElement { text: "160x120"; horRes: 160; verRes: 120 }
            }
            currentIndex: 2
            function getHorResolution() {
                return modes.get(currentIndex).horRes
            }
            function getVerResolution() {
                return modes.get(currentIndex).verRes
            }
        }

        Label {
            id: label_exposure
            x: parent.width / 3 + 30
            anchors.top: label_resolution.top
            text: "Exposure:"
        }
        Slider {
            id: slider_exposure
            anchors.left: label_exposure.left
            y: (parent.height - label_resolution.height - height) / 5 * 4
            width: parent.width / 3 - 60
            value: 100
            stepSize: 10
            minimumValue: 0
            maximumValue: 10000
        }
        ComboBox {
            id: combo_exposure
            anchors.left: label_exposure.left
            anchors.top: combo_resolution.top
            // see enum v4l2_exposure_auto_type in "linux/videodev2.h"
            model: [ "Auto", "Manual", "Shutter", "Aperture" ]
            currentIndex: 0
            onCurrentIndexChanged: {
                if (currentIndex == 0)
                    slider_exposure.enabled = false
                else
                    slider_exposure.enabled = true
            }
        }

        Label {
            id: label_quality
            x: parent.width / 3 * 2 + 10
            anchors.top: label_resolution.top
            text: ""
            function updateLabel(quality) {
                text = "Quality: " + quality + "%"
            }
        }
        Slider {
            id: slider_quality
            anchors.left: label_quality.left
            anchors.verticalCenter: combo_resolution.verticalCenter
            width: parent.width / 3 - 60
            tickmarksEnabled: false
            value: 70
            stepSize: 1
            minimumValue: 0
            maximumValue: 100
            onValueChanged: {
                label_quality.updateLabel(value)
            }
        }

        Label {
            id: label_camera
            x: -25
            anchors.verticalCenter: parent.verticalCenter
            text: "Camera"
            font.bold: true
            font.family: "Courier New"
            font.pointSize: 21
            rotation: -90
        }
    }

    Rectangle {
        id: rectangle_connect
        anchors.top: rectangle_camera.bottom
        height: parent.height / 3
        width: parent.width
        color: "#2da5c6"

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
                var url = ""
                if (radio_button_IPv6.checked)
                    url = text_IPv6_RPi_URL.text
                else
                    url = text_IPv4_RPi_IP.text
                connectRPi.tryDirectConnectToRPi(url, Number(textRPiPort.text),
                                                 combo_resolution.getHorResolution(), combo_resolution.getVerResolution(),
                                                 combo_exposure.currentIndex, slider_exposure.value,
                                                 slider_quality.value)
            }
        }
    }
}
