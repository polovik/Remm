import QtQuick 2.1

Rectangle {
    id: mainScreen
    width: 1000
    height: 700

    state: "CONNECTION"
    states: [
        State {
            name: "CONNECTION"
            PropertyChanges { target: screenControlPanel; x: 0; z: 1}
            PropertyChanges { target: screenSettings; z: 10}
        },
        State {
            name: "CONTROLLING"
            PropertyChanges { target: screenSettings; x: width; z: 1}
            PropertyChanges { target: screenControlPanel; z: 10}
        }
    ]
    transitions: [
        Transition {
            from: "CONNECTION"
            to: "CONTROLLING"
            SequentialAnimation {
                NumberAnimation { target: screenSettings; properties: "x"; easing.type: Easing.InBack; duration: 1000 }
                NumberAnimation { targets: [screenControlPanel, screenSettings]; property: "z" }
            }
        },
        Transition {
            from: "CONTROLLING"
            to: "CONNECTION"
            SequentialAnimation {
                NumberAnimation { targets: [screenControlPanel, screenSettings]; property: "z" }
                NumberAnimation { target: screenSettings; properties: "x"; easing.type: Easing.OutBack; duration: 1000 }
            }
        }
    ]

    ControlPanel {
        id: screenControlPanel
        x: 0
        y: 0
        width: parent.width
        height: parent.height
        Connections {
            onTryChangeSettings: {
                mainScreen.state = "CONNECTION"
            }
        }
    }

    Settings {
        id: screenSettings
        x: 0
        y: 0
        width: parent.width
        height: parent.height
        Connections {
            onConnectionEstablished: {
                screenControlPanel.makeDefaultSettings()
                mainScreen.state = "CONTROLLING"
            }
        }
    }
}
