/*
This is a UI file (.ui.qml) that is intended to be edited in Qt Design Studio only.
It is supposed to be strictly declarative and only uses a subset of QML. If you edit
this file manually, you might introduce QML code that is not supported by Qt Design Studio.
Check out https://doc.qt.io/qtcreator/creator-quick-ui-forms.html for details on .ui.qml files.
*/

import QtQuick
import QtQuick.Controls
import Admin_login

Rectangle {
    width: Constants.width
    height: Constants.height
    visible: true
    color: "#000cac7b"


    Rectangle {
        id: rectangle
        x: 682
        y: 312
        width: 580
        height: 493
        color: "#ffffff"
        radius: 24
        bottomRightRadius: 24
        bottomLeftRadius: 24
        topRightRadius: 24
        topLeftRadius: 24

        Rectangle {
            id: rectangle1
            color: "#e7eeec"
            radius: 16
            border.width: 0
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            anchors.topMargin: 16
            anchors.bottomMargin: 16
            bottomLeftRadius: 16
            topLeftRadius: 16

            Text {
                id: text1
                x: 24
                y: 16
                width: 238
                height: 42
                text: qsTr("Admin Login Detail")
                font.pixelSize: 17
                font.styleName: "Semibold Italic"
            }

            TextInput {
                id: textInput
                x: 183
                y: 114
                width: 224
                height: 38
                text: qsTr("UserName")
                font.pixelSize: 16
                font.family: "Arial"
                font.styleName: "Semibold"
            }

            TextInput {
                id: textInput1
                x: 183
                y: 172
                width: 224
                height: 43
                text: "Password"
                font.pixelSize: 16
                font.family: "Arial"
            }

            Button {
                id: login
                x: 203
                y: 249
                width: 158
                height: 35
                text: qsTr("Login")
                display: AbstractButton.IconOnly
            }
        }
    }
}

/*##^##
Designer {
    D{i:0}D{i:2;invisible:true}
}
##^##*/
