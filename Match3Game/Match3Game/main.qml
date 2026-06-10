import QtQuick 2.15
import QtQuick.Window 2.15

Window {
    id: root

    visible: true
    width: 980
    height: 860
    minimumWidth: 760
    minimumHeight: 700
    title: "Match3 Tools Demo"
    color: "#0d1720"

    property real boardPixels: Math.min(width - 96, height - 260, 620)
    property real cellPixels: Math.floor(boardPixels / boardModel.columns)

    function gemColor(colorId) {
        switch (colorId) {
        case 0:
            return "#ff7463"
        case 1:
            return "#f7b538"
        case 2:
            return "#36c9a4"
        case 3:
            return "#54a6ff"
        case 4:
            return "#c779ff"
        default:
            return "#8896a5"
        }
    }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#142534" }
            GradientStop { position: 0.52; color: "#10212f" }
            GradientStop { position: 1.0; color: "#0a131b" }
        }
    }

    Rectangle {
        width: 340
        height: 340
        radius: 170
        x: -70
        y: -80
        color: "#1d4252"
        opacity: 0.24
    }

    Rectangle {
        width: 280
        height: 280
        radius: 140
        anchors.right: parent.right
        anchors.rightMargin: -50
        anchors.bottom: parent.bottom
        anchors.bottomMargin: -70
        color: "#4f2a2d"
        opacity: 0.25
    }

    Item {
        anchors.fill: parent
        anchors.margins: 28

        Column {
            anchors.fill: parent
            spacing: 18

            Text {
                text: "Match-3 Tools Demo"
                color: "#f6efe4"
                font.family: "Trebuchet MS"
                font.pixelSize: 34
                font.bold: true
            }

            Text {
                width: parent.width
                text: "Tasks 10-14: tool generation, rocket line clear, bomb 5x5 clear, propeller top-box target, and rocket + propeller special swap."
                wrapMode: Text.WordWrap
                color: "#d5dee7"
                font.pixelSize: 15
                lineHeight: 1.2
            }

            Row {
                spacing: 12

                Rectangle {
                    width: 160
                    height: 74
                    radius: 20
                    color: "#1f3445"
                    border.color: "#355167"
                    border.width: 1

                    Column {
                        anchors.centerIn: parent
                        spacing: 2

                        Text {
                            text: "Boxes Left"
                            color: "#9cb0c1"
                            font.pixelSize: 13
                            horizontalAlignment: Text.AlignHCenter
                            width: parent.width
                        }

                        Text {
                            text: boardModel.remainingBoxes + " / " + boardModel.targetBoxes
                            color: "#fff5db"
                            font.pixelSize: 24
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            width: parent.width
                        }
                    }
                }

                Rectangle {
                    width: 150
                    height: 74
                    radius: 20
                    color: boardModel.inputLocked ? "#4f2f29" : "#17382d"
                    border.color: boardModel.inputLocked ? "#a46352" : "#2b7461"
                    border.width: 1

                    Column {
                        anchors.centerIn: parent
                        spacing: 2

                        Text {
                            text: "Input"
                            color: "#d4dddf"
                            font.pixelSize: 13
                            horizontalAlignment: Text.AlignHCenter
                            width: parent.width
                        }

                        Text {
                            text: boardModel.inputLocked ? "LOCKED" : "READY"
                            color: "#fff2d2"
                            font.pixelSize: 22
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            width: parent.width
                        }
                    }
                }

                Rectangle {
                    width: 170
                    height: 74
                    radius: 20
                    color: "#2f2340"
                    border.color: "#6c58a7"
                    border.width: 1

                    Column {
                        anchors.centerIn: parent
                        spacing: 2

                        Text {
                            text: "Seed"
                            color: "#c9c0dd"
                            font.pixelSize: 13
                            horizontalAlignment: Text.AlignHCenter
                            width: parent.width
                        }

                        Text {
                            text: boardModel.seed
                            color: "#f5eaff"
                            font.pixelSize: 22
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            width: parent.width
                        }
                    }
                }

                Rectangle {
                    height: 74
                    width: root.width - 612
                    radius: 20
                    color: "#1a2733"
                    border.color: "#33495d"
                    border.width: 1

                    Text {
                        anchors.fill: parent
                        anchors.margins: 18
                        text: boardModel.statusText
                        color: "#ebf1f5"
                        wrapMode: Text.WordWrap
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 14
                    }
                }
            }

            Item {
                width: parent.width
                height: parent.height - y

                Rectangle {
                    id: boardCard
                    width: root.cellPixels * boardModel.columns + 32
                    height: root.cellPixels * boardModel.rows + 32
                    radius: 28
                    color: "#142230"
                    border.color: "#345064"
                    border.width: 1
                    anchors.horizontalCenter: parent.horizontalCenter

                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: 12
                        radius: 22
                        color: "#101a24"
                    }

                    Grid {
                        id: boardGrid
                        anchors.centerIn: parent
                        columns: boardModel.columns
                        rowSpacing: 0
                        columnSpacing: 0

                        Repeater {
                            model: boardModel

                            delegate: Item {
                                width: root.cellPixels
                                height: root.cellPixels

                                Rectangle {
                                    anchors.fill: parent
                                    color: (index + Math.floor(index / boardModel.columns)) % 2 === 0 ? "#243746" : "#203240"
                                    border.color: "#2d4354"
                                    border.width: 1
                                }

                                Rectangle {
                                    anchors.fill: parent
                                    anchors.margins: 7
                                    radius: 18
                                    color: cellTypeName === "empty" ? "transparent" : "#16222d"
                                    border.color: cellTypeName === "empty" ? "#436075" : "#17232e"
                                    border.width: cellTypeName === "empty" ? 2 : 0
                                    opacity: cellTypeName === "empty" ? 0.7 : 1.0
                                }

                                Item {
                                    anchors.fill: parent
                                    anchors.margins: 8
                                    visible: cellTypeName === "normal"

                                    Rectangle {
                                        anchors.fill: parent
                                        radius: 16
                                        color: root.gemColor(cellColor)
                                    }

                                    Rectangle {
                                        width: parent.width * 0.54
                                        height: width
                                        radius: width / 2
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        anchors.top: parent.top
                                        anchors.topMargin: 7
                                        color: "#ffffff"
                                        opacity: 0.24
                                    }
                                }

                                Item {
                                    anchors.fill: parent
                                    anchors.margins: 8
                                    visible: cellTypeName === "box"

                                    Rectangle {
                                        anchors.fill: parent
                                        radius: 14
                                        color: "#8d5b33"
                                        border.color: "#d8a46b"
                                        border.width: 2
                                    }

                                    Rectangle {
                                        width: 4
                                        height: parent.height - 14
                                        anchors.centerIn: parent
                                        color: "#d8a46b"
                                    }

                                    Rectangle {
                                        width: parent.width - 14
                                        height: 4
                                        anchors.centerIn: parent
                                        color: "#d8a46b"
                                    }
                                }

                                Item {
                                    anchors.fill: parent
                                    anchors.margins: 8
                                    visible: cellTypeName === "rocketHorizontal" || cellTypeName === "rocketVertical"

                                    Rectangle {
                                        anchors.fill: parent
                                        radius: 16
                                        color: cellTypeName === "rocketHorizontal" ? "#ffd26a" : "#7fd2ff"
                                    }

                                    Item {
                                        anchors.fill: parent
                                        visible: cellTypeName === "rocketHorizontal"

                                        Rectangle {
                                            x: parent.width * 0.16
                                            y: parent.height * 0.33
                                            width: parent.width * 0.48
                                            height: parent.height * 0.34
                                            radius: 8
                                            color: "#fff8e7"
                                        }

                                        Rectangle {
                                            x: parent.width * 0.58
                                            y: parent.height * 0.25
                                            width: parent.width * 0.2
                                            height: parent.height * 0.5
                                            rotation: 45
                                            radius: 4
                                            color: "#ff6b57"
                                        }

                                        Rectangle {
                                            x: parent.width * 0.08
                                            y: parent.height * 0.2
                                            width: parent.width * 0.16
                                            height: parent.height * 0.18
                                            rotation: -28
                                            radius: 3
                                            color: "#26485c"
                                        }

                                        Rectangle {
                                            x: parent.width * 0.08
                                            y: parent.height * 0.62
                                            width: parent.width * 0.16
                                            height: parent.height * 0.18
                                            rotation: 28
                                            radius: 3
                                            color: "#26485c"
                                        }
                                    }

                                    Item {
                                        anchors.fill: parent
                                        visible: cellTypeName === "rocketVertical"

                                        Rectangle {
                                            x: parent.width * 0.33
                                            y: parent.height * 0.16
                                            width: parent.width * 0.34
                                            height: parent.height * 0.48
                                            radius: 8
                                            color: "#fff8e7"
                                        }

                                        Rectangle {
                                            x: parent.width * 0.25
                                            y: parent.height * 0.02
                                            width: parent.width * 0.5
                                            height: parent.height * 0.22
                                            rotation: 45
                                            radius: 4
                                            color: "#ff8a57"
                                        }

                                        Rectangle {
                                            x: parent.width * 0.2
                                            y: parent.height * 0.74
                                            width: parent.width * 0.18
                                            height: parent.height * 0.16
                                            rotation: -28
                                            radius: 3
                                            color: "#20435e"
                                        }

                                        Rectangle {
                                            x: parent.width * 0.62
                                            y: parent.height * 0.74
                                            width: parent.width * 0.18
                                            height: parent.height * 0.16
                                            rotation: 28
                                            radius: 3
                                            color: "#20435e"
                                        }
                                    }
                                }

                                Item {
                                    anchors.fill: parent
                                    anchors.margins: 8
                                    visible: cellTypeName === "bomb"

                                    Rectangle {
                                        width: parent.width * 0.76
                                        height: width
                                        radius: width / 2
                                        anchors.centerIn: parent
                                        color: "#262833"
                                        border.color: "#ffb347"
                                        border.width: 3
                                    }

                                    Rectangle {
                                        width: parent.width * 0.32
                                        height: parent.height * 0.08
                                        radius: 3
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        anchors.top: parent.top
                                        anchors.topMargin: 8
                                        rotation: -24
                                        color: "#f2c078"
                                    }

                                    Rectangle {
                                        width: parent.width * 0.18
                                        height: width
                                        radius: width / 2
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        anchors.top: parent.top
                                        anchors.topMargin: 2
                                        color: "#fff1a8"
                                    }

                                    Rectangle {
                                        width: parent.width * 0.24
                                        height: width
                                        radius: width / 2
                                        anchors.centerIn: parent
                                        color: "#ffcf63"
                                        opacity: 0.35
                                    }
                                }

                                Item {
                                    anchors.fill: parent
                                    anchors.margins: 8
                                    visible: cellTypeName === "propeller"

                                    Rectangle {
                                        anchors.fill: parent
                                        radius: 16
                                        color: "#d7fff4"
                                        opacity: 0.18
                                    }

                                    Rectangle {
                                        width: parent.width * 0.26
                                        height: parent.height * 0.62
                                        radius: 8
                                        anchors.centerIn: parent
                                        color: "#7de0b1"
                                    }

                                    Rectangle {
                                        width: parent.width * 0.26
                                        height: parent.height * 0.62
                                        radius: 8
                                        anchors.centerIn: parent
                                        rotation: 90
                                        color: "#ffd66c"
                                    }

                                    Rectangle {
                                        width: parent.width * 0.22
                                        height: parent.height * 0.46
                                        radius: 8
                                        anchors.centerIn: parent
                                        rotation: 45
                                        color: "#56c6ff"
                                    }

                                    Rectangle {
                                        width: parent.width * 0.22
                                        height: parent.height * 0.46
                                        radius: 8
                                        anchors.centerIn: parent
                                        rotation: -45
                                        color: "#ff8a7f"
                                    }

                                    Rectangle {
                                        width: parent.width * 0.26
                                        height: width
                                        radius: width / 2
                                        anchors.centerIn: parent
                                        color: "#ffffff"
                                        border.color: "#4d6f7a"
                                        border.width: 2
                                    }
                                }

                                Rectangle {
                                    anchors.fill: parent
                                    anchors.margins: 4
                                    radius: 20
                                    color: "transparent"
                                    border.color: selected ? "#fff4da" : "transparent"
                                    border.width: 3
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    enabled: !boardModel.inputLocked
                                    onClicked: boardModel.clickCell(cellRow, cellColumn)
                                }
                            }
                        }
                    }

                    Rectangle {
                        anchors.fill: parent
                        radius: 28
                        color: "#081015"
                        opacity: boardModel.inputLocked ? 0.24 : 0.0
                        visible: opacity > 0
                    }

                    Rectangle {
                        anchors.fill: parent
                        radius: 28
                        visible: boardModel.gameWon
                        color: "#091219"
                        opacity: 0.86

                        Column {
                            anchors.centerIn: parent
                            spacing: 10

                            Text {
                                text: "GREAT"
                                color: "#ffe3a3"
                                font.family: "Trebuchet MS"
                                font.pixelSize: 52
                                font.bold: true
                                horizontalAlignment: Text.AlignHCenter
                                width: parent.width
                            }

                            Text {
                                text: "All target boxes are gone."
                                color: "#edf5ff"
                                font.pixelSize: 18
                                horizontalAlignment: Text.AlignHCenter
                                width: parent.width
                            }

                            Text {
                                text: "Reset with the same seed to replay, or move to the next seed."
                                color: "#b7c6d6"
                                font.pixelSize: 14
                                horizontalAlignment: Text.AlignHCenter
                                width: parent.width
                            }
                        }
                    }
                }

                Row {
                    spacing: 14
                    anchors.top: boardCard.bottom
                    anchors.topMargin: 18
                    anchors.horizontalCenter: parent.horizontalCenter

                    Rectangle {
                        width: 172
                        height: 52
                        radius: 18
                        color: "#f0dcc0"

                        Text {
                            anchors.centerIn: parent
                            text: "Reset Same Seed"
                            color: "#33271d"
                            font.pixelSize: 16
                            font.bold: true
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: boardModel.resetBoard()
                        }
                    }

                    Rectangle {
                        width: 156
                        height: 52
                        radius: 18
                        color: "#2f6a62"

                        Text {
                            anchors.centerIn: parent
                            text: "Next Seed"
                            color: "#edf7f3"
                            font.pixelSize: 16
                            font.bold: true
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: boardModel.nextSeed()
                        }
                    }
                }
            }
        }
    }
}
