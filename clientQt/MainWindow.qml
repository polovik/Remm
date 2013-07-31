import QtQuick 2.1

Rectangle {
    id: mainScreen
    width: 700
    height: 500

    state: "CONNECTION"
    states: [
        State {
            name: "CONNECTION"
            PropertyChanges { target: screenControlPanel; z: 1}
            PropertyChanges { target: screenSettings; z: 10}
        },
        State {
            name: "CONTROLLING"
            PropertyChanges { target: screenSettings; x: width; z: 1}
            PropertyChanges { target: screenControlPanel; z: 10}
        }
    ]
    transitions: Transition {
        from: "CONNECTION"
        to: "CONTROLLING"
        SequentialAnimation {
            NumberAnimation { target: screenSettings; properties: "x"; easing.type: Easing.InOutBack; duration: 1000 }
            NumberAnimation { targets: [screenControlPanel, screenSettings]; property: "z" }
        }
    }

    ControlPanel {
        id: screenControlPanel
        x: 0
        y: 0
        width: parent.width
        height: parent.height
    }

    Settings {
        id: screenSettings
        x: 0
        y: 0
        width: parent.width
        height: parent.height
        Connections {
            onConnectionEstablished: {
                mainScreen.state = "CONTROLLING"
            }
        }
    }
}
