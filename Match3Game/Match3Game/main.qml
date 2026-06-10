import QtQuick 2.15
import QtQuick.Window 2.15

Window {
    id: root

    visible: true
    width: 860
    height: 1120
    minimumWidth: 760
    minimumHeight: 920
    title: "Match3 Demo"
    color: "#081421"

    property real boardPixels: Math.min(width - 120, height - 360, 640)
    property real cellPixels: Math.floor(boardPixels / boardModel.columns)

    function pieceColor(colorId) {
        switch (colorId) {
        case 0:
            return "#ff3138"
        case 1:
            return "#ffc72f"
        case 2:
            return "#1fdd2f"
        case 3:
            return "#2878ff"
        case 4:
            return "#ff8a1f"
        default:
            return "#8ca0ba"
        }
    }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#112a45" }
            GradientStop { position: 0.35; color: "#0a223a" }
            GradientStop { position: 0.72; color: "#08192b" }
            GradientStop { position: 1.0; color: "#06111b" }
        }
    }

    Rectangle {
        anchors.fill: parent
        opacity: 0.14
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#7ec9ff" }
            GradientStop { position: 0.16; color: "transparent" }
            GradientStop { position: 0.34; color: "#7ec9ff" }
            GradientStop { position: 0.5; color: "transparent" }
            GradientStop { position: 0.68; color: "#7ec9ff" }
            GradientStop { position: 1.0; color: "transparent" }
        }
        rotation: 9
        scale: 1.35
    }

    Repeater {
        model: 40
        delegate: Rectangle {
            width: 6 + (index % 4)
            height: width
            radius: width / 2
            color: "#f6fbff"
            opacity: 0.75
            x: (index * 137) % root.width
            y: (index * 211) % root.height
        }
    }

    Item {
        anchors.fill: parent
        anchors.margins: 36

        Column {
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width
            spacing: 18

            Rectangle {
                width: 310
                height: 88
                radius: 32
                anchors.horizontalCenter: parent.horizontalCenter
                color: "#fff3e6"
                border.color: "#efd0bc"
                border.width: 2

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 10
                    radius: 24
                    color: "#ffdcb7"
                }

                Row {
                    anchors.centerIn: parent
                    spacing: 14

                    Rectangle {
                        width: 42
                        height: 42
                        radius: 12
                        color: "#ef7f1a"
                        border.color: "#cc5f03"
                        border.width: 2

                        Rectangle {
                            width: 5
                            height: parent.height
                            anchors.horizontalCenter: parent.horizontalCenter
                            color: "#ffac55"
                        }

                        Rectangle {
                            width: parent.width
                            height: 5
                            anchors.verticalCenter: parent.verticalCenter
                            color: "#ffac55"
                        }
                    }

                    Text {
                        text: boardModel.remainingBoxes
                        color: "#1b2b4a"
                        font.family: "Trebuchet MS"
                        font.pixelSize: 34
                        font.bold: true
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: boardModel.statusText
                width: 520
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
                color: "#e9f3ff"
                font.pixelSize: 16
                lineHeight: 1.2
            }

            Rectangle {
                id: boardFrame
                width: root.cellPixels * boardModel.columns + 22
                height: root.cellPixels * boardModel.rows + 22
                radius: 20
                anchors.horizontalCenter: parent.horizontalCenter
                color: "#67d1ff"
                border.color: "#b4efff"
                border.width: 3

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 6
                    radius: 15
                    color: "#8bdcff"
                }

                Grid {
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
                                color: ((index + Math.floor(index / boardModel.columns)) % 2 === 0) ? "#b9e6ff" : "#acdfff"
                                border.color: "#8fcef8"
                                border.width: 1
                            }

                            Rectangle {
                                anchors.fill: parent
                                anchors.margins: 5
                                radius: 14
                                color: cellTypeName === "empty" ? "transparent" : "#ffffff"
                                opacity: cellTypeName === "empty" ? 0.25 : 0.12
                            }

                            Item {
                                anchors.fill: parent
                                anchors.margins: 7
                                visible: cellTypeName === "normal" && cellColor === 0

                                Rectangle {
                                    x: parent.width * 0.12
                                    y: parent.height * 0.22
                                    width: parent.width * 0.76
                                    height: parent.height * 0.5
                                    radius: 16
                                    color: "#ff2f38"
                                    border.color: "#d70e18"
                                    border.width: 2
                                }

                                Rectangle {
                                    width: parent.width * 0.4
                                    height: parent.height * 0.18
                                    radius: 8
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    anchors.top: parent.top
                                    anchors.topMargin: 8
                                    color: "#ff8a8c"
                                    opacity: 0.45
                                }
                            }

                            Item {
                                anchors.fill: parent
                                anchors.margins: 7
                                visible: cellTypeName === "normal" && cellColor === 1

                                Rectangle {
                                    x: parent.width * 0.22
                                    y: parent.height * 0.12
                                    width: parent.width * 0.56
                                    height: parent.height * 0.68
                                    radius: 18
                                    color: "#ffc228"
                                    border.color: "#e39f00"
                                    border.width: 2
                                }

                                Rectangle {
                                    width: parent.width * 0.24
                                    height: parent.height * 0.12
                                    radius: 6
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    anchors.top: parent.top
                                    anchors.topMargin: 10
                                    color: "#ffe18b"
                                    opacity: 0.65
                                }
                            }

                            Item {
                                anchors.fill: parent
                                anchors.margins: 7
                                visible: cellTypeName === "normal" && cellColor === 2

                                Rectangle {
                                    width: parent.width * 0.62
                                    height: parent.height * 0.72
                                    radius: width / 2
                                    anchors.centerIn: parent
                                    rotation: 38
                                    color: "#18d82d"
                                    border.color: "#11aa22"
                                    border.width: 2
                                }

                                Rectangle {
                                    width: 5
                                    height: parent.height * 0.22
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    anchors.top: parent.top
                                    anchors.topMargin: 8
                                    color: "#0f8a1d"
                                    rotation: -20
                                }
                            }

                            Item {
                                anchors.fill: parent
                                anchors.margins: 7
                                visible: cellTypeName === "normal" && cellColor === 3

                                Rectangle {
                                    x: parent.width * 0.16
                                    y: parent.height * 0.14
                                    width: parent.width * 0.68
                                    height: parent.height * 0.68
                                    radius: 14
                                    color: "#2b75ff"
                                    border.color: "#0f57d7"
                                    border.width: 2
                                }

                                Rectangle {
                                    width: parent.width * 0.18
                                    height: width
                                    radius: width / 2
                                    anchors.centerIn: parent
                                    color: "#5fa6ff"
                                    opacity: 0.32
                                }
                            }

                            Item {
                                anchors.fill: parent
                                anchors.margins: 7
                                visible: cellTypeName === "normal" && cellColor === 4

                                Rectangle {
                                    width: parent.width * 0.54
                                    height: parent.height * 0.7
                                    radius: 20
                                    anchors.centerIn: parent
                                    color: "#ff8a1f"
                                    border.color: "#d16606"
                                    border.width: 2
                                }

                                Rectangle {
                                    width: parent.width * 0.14
                                    height: parent.height * 0.14
                                    radius: width / 2
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    anchors.top: parent.top
                                    anchors.topMargin: 7
                                    color: "#ffd29f"
                                    opacity: 0.7
                                }
                            }

                            Item {
                                anchors.fill: parent
                                anchors.margins: 8
                                visible: cellTypeName === "box"

                                Rectangle {
                                    anchors.fill: parent
                                    radius: 12
                                    color: "#ef7f1a"
                                    border.color: "#cf5f02"
                                    border.width: 2
                                }

                                Rectangle {
                                    width: 5
                                    height: parent.height
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    color: "#ffb15c"
                                }

                                Rectangle {
                                    width: parent.width
                                    height: 5
                                    anchors.verticalCenter: parent.verticalCenter
                                    color: "#ffb15c"
                                }
                            }

                            Item {
                                anchors.fill: parent
                                anchors.margins: 8
                                visible: cellTypeName === "rocketHorizontal" || cellTypeName === "rocketVertical"

                                Rectangle {
                                    anchors.fill: parent
                                    radius: 14
                                    color: "#ffa62a"
                                    border.color: "#d36d00"
                                    border.width: 2
                                }

                                Rectangle {
                                    width: cellTypeName === "rocketHorizontal" ? parent.width * 0.58 : parent.width * 0.28
                                    height: cellTypeName === "rocketHorizontal" ? parent.height * 0.28 : parent.height * 0.58
                                    anchors.centerIn: parent
                                    radius: 8
                                    color: "#6f3fdc"
                                }

                                Rectangle {
                                    width: cellTypeName === "rocketHorizontal" ? parent.width * 0.22 : parent.width * 0.5
                                    height: cellTypeName === "rocketHorizontal" ? parent.height * 0.46 : parent.height * 0.22
                                    anchors.centerIn: parent
                                    radius: 6
                                    rotation: 45
                                    color: "#fff3e0"
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
                                    color: "#2b3443"
                                    border.color: "#ffb43a"
                                    border.width: 3
                                }

                                Rectangle {
                                    width: parent.width * 0.26
                                    height: 6
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    anchors.top: parent.top
                                    anchors.topMargin: 6
                                    radius: 3
                                    rotation: -24
                                    color: "#ffd28e"
                                }

                                Rectangle {
                                    width: parent.width * 0.16
                                    height: width
                                    radius: width / 2
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    anchors.top: parent.top
                                    anchors.topMargin: 2
                                    color: "#fff3aa"
                                }
                            }

                            Item {
                                anchors.fill: parent
                                anchors.margins: 8
                                visible: cellTypeName === "propeller"

                                Rectangle {
                                    width: parent.width * 0.28
                                    height: parent.height * 0.68
                                    anchors.centerIn: parent
                                    radius: 8
                                    color: "#ffd144"
                                }

                                Rectangle {
                                    width: parent.width * 0.28
                                    height: parent.height * 0.68
                                    anchors.centerIn: parent
                                    radius: 8
                                    rotation: 90
                                    color: "#ff4a4f"
                                }

                                Rectangle {
                                    width: parent.width * 0.2
                                    height: parent.height * 0.46
                                    anchors.centerIn: parent
                                    radius: 8
                                    rotation: 45
                                    color: "#2ad561"
                                }

                                Rectangle {
                                    width: parent.width * 0.2
                                    height: parent.height * 0.46
                                    anchors.centerIn: parent
                                    radius: 8
                                    rotation: -45
                                    color: "#2e7dff"
                                }

                                Rectangle {
                                    width: parent.width * 0.26
                                    height: width
                                    radius: width / 2
                                    anchors.centerIn: parent
                                    color: "#fff7de"
                                    border.color: "#ad6d2c"
                                    border.width: 2
                                }
                            }

                            Rectangle {
                                anchors.fill: parent
                                anchors.margins: 3
                                radius: 16
                                color: "transparent"
                                border.color: selected ? "#fff4cc" : "transparent"
                                border.width: 4
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
                    radius: 20
                    color: "#02111d"
                    opacity: boardModel.inputLocked && !boardModel.gameWon ? 0.18 : 0.0
                    visible: opacity > 0
                }

                Rectangle {
                    anchors.fill: parent
                    radius: 20
                    visible: boardModel.gameWon
                    color: "#03121d"
                    opacity: 0.88

                    Column {
                        anchors.centerIn: parent
                        spacing: 14

                        Text {
                            text: "Great"
                            color: "#ffd53b"
                            font.family: "Trebuchet MS"
                            font.pixelSize: 58
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            width: parent.width
                        }

                        Rectangle {
                            width: 110
                            height: 110
                            radius: 28
                            anchors.horizontalCenter: parent.horizontalCenter
                            color: "#ef7f1a"
                            border.color: "#cf5f02"
                            border.width: 3

                            Rectangle {
                                width: 8
                                height: parent.height
                                anchors.horizontalCenter: parent.horizontalCenter
                                color: "#ffb15c"
                            }

                            Rectangle {
                                width: parent.width
                                height: 8
                                anchors.verticalCenter: parent.verticalCenter
                                color: "#ffb15c"
                            }
                        }

                        Text {
                            text: "所有箱子都清完了"
                            color: "#eef7ff"
                            font.pixelSize: 20
                            horizontalAlignment: Text.AlignHCenter
                            width: parent.width
                        }
                    }
                }
            }

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 14

                Rectangle {
                    width: 150
                    height: 52
                    radius: 18
                    color: "#fff0dc"
                    border.color: "#ebd1b5"
                    border.width: 2

                    Text {
                        anchors.centerIn: parent
                        text: "同 Seed 重开"
                        color: "#3c2d20"
                        font.pixelSize: 16
                        font.bold: true
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: boardModel.resetBoard()
                    }
                }

                Rectangle {
                    width: 138
                    height: 52
                    radius: 18
                    color: "#5b7cff"
                    border.color: "#87a2ff"
                    border.width: 2

                    Text {
                        anchors.centerIn: parent
                        text: "下一 Seed"
                        color: "#f5f8ff"
                        font.pixelSize: 16
                        font.bold: true
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: boardModel.nextSeed()
                    }
                }

                Rectangle {
                    width: 156
                    height: 52
                    radius: 18
                    color: "#142a42"
                    border.color: "#4f6d90"
                    border.width: 2

                    Text {
                        anchors.centerIn: parent
                        text: "Seed " + boardModel.seed
                        color: "#ecf4ff"
                        font.pixelSize: 16
                        font.bold: true
                    }
                }
            }
        }
    }
}
