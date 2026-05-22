import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    visible: true
    width: 1280
    height: 780
    title: "NMEA Monitor — Real-Time Sensor Dashboard"
    color: "#0D1117"

    // ── Colour system ──────────────────────────────────────────────────────
    // CRITICAL : red family   — stands out clearly
    // WARNING  : amber family — clearly distinct from critical
    // NORMAL   : green family

    function alertFg(level) {
        if (level === 2) return "#FF5555"   // critical — bright red
        if (level === 1) return "#F0C040"   // warning  — amber/yellow
        return "#4CFF91"                    // normal   — green
    }
    function alertBg(level) {
        if (level === 2) return "#4D0000"   // critical bg
        if (level === 1) return "#2D1F00"   // warning bg
        return "#003D1A"                    // normal bg
    }
    function alertBorder(level) {
        if (level === 2) return "#CC2222"
        if (level === 1) return "#C09020"
        return "#2CCC71"
    }
    function alertLabel(level) {
        if (level === 2) return "CRITICAL"
        if (level === 1) return "WARNING"
        return "NORMAL"
    }
    function fmtThresh(v) {
        return Math.abs(v) >= 1e8 ? "—" : v.toFixed(1)
    }

    // ── Header ─────────────────────────────────────────────────────────────
    Rectangle {
        id: header
        anchors { top: parent.top; left: parent.left; right: parent.right }
        height: 56
        color: "#161B22"

        Row {
            anchors { verticalCenter: parent.verticalCenter; left: parent.left; leftMargin: 20 }
            spacing: 12

            Rectangle {
                width: 10; height: 10; radius: 5; color: "#4CFF91"
                anchors.verticalCenter: parent.verticalCenter
                SequentialAnimation on opacity {
                    loops: Animation.Infinite
                    NumberAnimation { to: 0.2; duration: 800 }
                    NumberAnimation { to: 1.0; duration: 800 }
                }
            }
            Text {
                text: "NMEA Monitor  ·  Real-Time Sensor Dashboard"
                color: "#E6EDF3"
                font.pixelSize: 16
                font.bold: true
            }
        }

        Text {
            id: clockText
            anchors { verticalCenter: parent.verticalCenter; right: parent.right; rightMargin: 20 }
            color: "#7D8590"
            font.family: "Courier New"
            font.pixelSize: 13
            text: Qt.formatDateTime(new Date(), "yyyy-MM-dd  HH:mm:ss")
            Timer {
                interval: 1000; running: true; repeat: true
                onTriggered: clockText.text = Qt.formatDateTime(new Date(), "yyyy-MM-dd  HH:mm:ss")
            }
        }
    }

    // ── Main layout ─────────────────────────────────────────────────────────
    RowLayout {
        anchors {
            top: header.bottom; left: parent.left; right: parent.right; bottom: footer.top
            margins: 16
        }
        spacing: 16

        // ── Left column ────────────────────────────────────────────────────
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            // ── Sensor channel table ───────────────────────────────────────
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: tableHeader.height + 12 + sensorModel.count * 42 + 16
                Layout.minimumHeight: 120
                color: "#161B22"
                radius: 8

                // Column widths — must match delegate
                readonly property var colW: [170, 85, 55, 80, 85, 75, 82, 96, 160]

                Column {
                    id: tableHeader
                    anchors { top: parent.top; left: parent.left; right: parent.right
                              topMargin: 12; leftMargin: 16 }
                    spacing: 6

                    Text {
                        text: "SENSOR CHANNELS"
                        color: "#7D8590"
                        font.pixelSize: 11; font.bold: true; font.letterSpacing: 1.5
                    }

                    Row {
                        Repeater {
                            model: ["CHANNEL","VALUE","UNIT","WARN LOW","WARN HIGH","CRIT LOW","CRIT HIGH","STATUS","LAST UPDATE"]
                            delegate: Text {
                                width: parent.parent.parent.colW[index]
                                text: modelData
                                color: "#7D8590"
                                font.pixelSize: 10; font.bold: true
                            }
                        }
                    }
                    Rectangle { width: parent.width - 16; height: 1; color: "#30363D" }
                }

                ListView {
                    anchors {
                        top: tableHeader.bottom; topMargin: 6
                        left: parent.left; leftMargin: 16
                        right: parent.right; rightMargin: 4
                        bottom: parent.bottom; bottomMargin: 8
                    }
                    model: sensorModel
                    clip: true
                    spacing: 2

                    delegate: Rectangle {
                        readonly property var colW: [170, 85, 55, 80, 85, 75, 82, 96, 160]
                        width: ListView.view.width
                        height: 40
                        color: alertLevel > 0
                            ? root.alertBg(alertLevel)
                            : (index % 2 === 0 ? "#0D1117" : "#161B22")
                        radius: 4
                        Behavior on color { ColorAnimation { duration: 400 } }

                        // Left accent bar for alerts
                        Rectangle {
                            visible: alertLevel > 0
                            width: 3; height: parent.height
                            color: root.alertBorder(alertLevel)
                            radius: 2
                        }

                        Row {
                            anchors { verticalCenter: parent.verticalCenter; left: parent.left; leftMargin: 6 }
                            spacing: 0

                            Text {
                                width: colW[0]; text: sensorName
                                color: "#E6EDF3"
                                font.family: "Courier New"; font.pixelSize: 12
                                elide: Text.ElideRight
                            }
                            Text {
                                width: colW[1]
                                text: hasData ? value.toFixed(2) : "—"
                                color: hasData ? root.alertFg(alertLevel) : "#484F58"
                                font.family: "Courier New"; font.pixelSize: 12; font.bold: alertLevel > 0
                                Behavior on color { ColorAnimation { duration: 400 } }
                            }
                            Text {
                                width: colW[2]; text: unit
                                color: "#7D8590"; font.family: "Courier New"; font.pixelSize: 12
                            }
                            Text {
                                width: colW[3]; text: root.fmtThresh(warnLow)
                                color: "#C09020"; font.family: "Courier New"; font.pixelSize: 11
                            }
                            Text {
                                width: colW[4]; text: root.fmtThresh(warnHigh)
                                color: "#C09020"; font.family: "Courier New"; font.pixelSize: 11
                            }
                            Text {
                                width: colW[5]; text: root.fmtThresh(critLow)
                                color: "#CC2222"; font.family: "Courier New"; font.pixelSize: 11
                            }
                            Text {
                                width: colW[6]; text: root.fmtThresh(critHigh)
                                color: "#CC2222"; font.family: "Courier New"; font.pixelSize: 11
                            }
                            // Status badge
                            Rectangle {
                                width: 84; height: 22; radius: 4
                                color: hasData ? root.alertBg(alertLevel) : "#1C2128"
                                border.color: hasData ? root.alertBorder(alertLevel) : "#30363D"
                                border.width: 1
                                Behavior on border.color { ColorAnimation { duration: 400 } }
                                Text {
                                    anchors.centerIn: parent
                                    text: hasData ? root.alertLabel(alertLevel) : "NO DATA"
                                    color: hasData ? root.alertFg(alertLevel) : "#484F58"
                                    font.pixelSize: 10; font.bold: true; font.letterSpacing: 1
                                }
                            }
                            Text {
                                width: colW[8]; leftPadding: 10
                                text: hasData ? timestamp : "no data"
                                color: hasData ? "#7D8590" : "#484F58"
                                font.family: "Courier New"; font.pixelSize: 11
                                font.italic: !hasData
                            }
                        }
                    }
                    ScrollBar.vertical: ScrollBar {}
                }
            }

            // ── Event log ──────────────────────────────────────────────────
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: 180
                color: "#161B22"
                radius: 8

                // Column widths — must match delegate
                readonly property var colW: [72, 160, 100, 80, 68, 80, 0]

                Column {
                    id: logHeader
                    anchors { top: parent.top; left: parent.left; right: parent.right
                              topMargin: 10; leftMargin: 16 }
                    spacing: 4

                    Text {
                        text: "EVENT LOG"
                        color: "#7D8590"
                        font.pixelSize: 11; font.bold: true; font.letterSpacing: 1.5
                    }

                    Row {
                        Repeater {
                            model: ["TIME", "SENSOR", "VALUE", "LEVEL", "STATUS", "DURATION", "THRESHOLD"]
                            delegate: Text {
                                width: index < 6
                                    ? parent.parent.parent.colW[index]
                                    : (parent.parent.parent.width - 16 - 72 - 160 - 100 - 80 - 68 - 80)
                                text: modelData
                                color: "#7D8590"
                                font.pixelSize: 10; font.bold: true
                            }
                        }
                    }
                    Rectangle { width: parent.width - 16; height: 1; color: "#30363D" }
                }

                ListView {
                    anchors {
                        top: logHeader.bottom; topMargin: 4
                        left: parent.left; leftMargin: 16
                        right: parent.right; rightMargin: 4
                        bottom: parent.bottom; bottomMargin: 6
                    }
                    model: eventLogModel
                    clip: true
                    spacing: 1

                    delegate: Rectangle {
                        readonly property bool isActive: eventStatus === "ACTIVE"
                        width: ListView.view.width
                        height: 28
                        color: isActive ? root.alertBg(eventLevel) : (index % 2 === 0 ? "#0D1117" : "#161B22")
                        radius: 3

                        // Left accent bar for active events
                        Rectangle {
                            visible: isActive
                            width: 3; height: parent.height
                            color: root.alertBorder(eventLevel)
                            radius: 2
                        }

                        Row {
                            anchors { verticalCenter: parent.verticalCenter; left: parent.left; leftMargin: 6 }
                            spacing: 0

                            // TIME
                            Text {
                                width: 72; text: eventTime
                                color: "#7D8590"; font.family: "Courier New"; font.pixelSize: 11
                            }
                            // SENSOR
                            Text {
                                width: 160; text: eventSensor
                                color: "#E6EDF3"
                                font.family: "Courier New"; font.pixelSize: 11
                                elide: Text.ElideRight
                            }
                            // VALUE
                            Text {
                                width: 100
                                text: eventValue.toFixed(2) + " " + eventUnit
                                color: root.alertFg(eventLevel)
                                font.family: "Courier New"; font.pixelSize: 11; font.bold: true
                            }
                            // LEVEL badge
                            Rectangle {
                                width: 72; height: 18; radius: 3
                                anchors.verticalCenter: parent.verticalCenter
                                color: root.alertBg(eventLevel)
                                border.color: root.alertBorder(eventLevel); border.width: 1
                                Text {
                                    anchors.centerIn: parent
                                    text: eventLevel === 2 ? "CRITICAL" : "WARNING"
                                    color: root.alertFg(eventLevel)
                                    font.pixelSize: 9; font.bold: true
                                }
                            }
                            // STATUS badge
                            Rectangle {
                                width: 60; height: 18; radius: 3
                                anchors.verticalCenter: parent.verticalCenter
                                color: isActive ? "#0D2A4D" : "#1C2128"
                                border.color: isActive ? "#58A6FF" : "#484F58"; border.width: 1
                                Text {
                                    anchors.centerIn: parent
                                    text: eventStatus
                                    color: isActive ? "#58A6FF" : "#7D8590"
                                    font.pixelSize: 9; font.bold: true
                                }
                            }
                            // DURATION
                            Text {
                                width: 80; leftPadding: 8
                                text: eventDuration
                                color: isActive ? "#58A6FF" : "#7D8590"
                                font.family: "Courier New"; font.pixelSize: 11; font.bold: isActive
                            }
                            // THRESHOLD
                            Text {
                                width: parent.parent.width - 72 - 160 - 100 - 72 - 60 - 80 - 8
                                text: eventThresh
                                color: "#484F58"; font.family: "Courier New"; font.pixelSize: 10
                                elide: Text.ElideRight
                            }
                        }
                    }
                    ScrollBar.vertical: ScrollBar {}
                }
            }
        }

        // ── Right panel ────────────────────────────────────────────────────
        Rectangle {
            Layout.preferredWidth: 230
            Layout.fillHeight: true
            color: "#161B22"
            radius: 8

            Column {
                anchors { fill: parent; margins: 16 }
                spacing: 14

                // Alert summary
                Text {
                    text: "ALERT SUMMARY"
                    color: "#7D8590"
                    font.pixelSize: 11; font.bold: true; font.letterSpacing: 1.5
                }

                Column {
                    width: parent.width
                    spacing: 8

                    Row {
                        width: parent.width; spacing: 8
                        Rectangle {
                            width: 10; height: 10; radius: 2; color: "#FF5555"
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text {
                            text: "CRITICAL"; color: "#FF5555"
                            font.pixelSize: 11; font.bold: true; width: 75
                        }
                        Rectangle {
                            width: 36; height: 20; radius: 3
                            color: "#4D0000"; border.color: "#CC2222"; border.width: 1
                            Text {
                                anchors.centerIn: parent
                                text: sensorModel.criticalCount
                                color: "#FF5555"; font.pixelSize: 12; font.bold: true
                            }
                        }
                    }

                    Row {
                        width: parent.width; spacing: 8
                        Rectangle {
                            width: 10; height: 10; radius: 2; color: "#F0C040"
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text {
                            text: "WARNING"; color: "#F0C040"
                            font.pixelSize: 11; font.bold: true; width: 75
                        }
                        Rectangle {
                            width: 36; height: 20; radius: 3
                            color: "#2D1F00"; border.color: "#C09020"; border.width: 1
                            Text {
                                anchors.centerIn: parent
                                text: sensorModel.warningCount
                                color: "#F0C040"; font.pixelSize: 12; font.bold: true
                            }
                        }
                    }
                }

                Rectangle { width: parent.width; height: 1; color: "#30363D" }

                // System info
                Text {
                    text: "SYSTEM"
                    color: "#7D8590"
                    font.pixelSize: 11; font.bold: true; font.letterSpacing: 1.5
                }

                Column {
                    width: parent.width
                    spacing: 5

                    Repeater {
                        model: [
                            { label: "Channels",  value: sensorModel.count + " active" },
                            { label: "Port",      value: serialPort         },
                            { label: "Source",    value: "NMEA-0183"        },
                            { label: "Standard",  value: "IEC 61162-1"      },
                            { label: "Config",    value: "sensors.json"     },
                        ]
                        delegate: Row {
                            width: parent.width
                            Text {
                                width: 90; text: modelData.label
                                color: "#7D8590"; font.pixelSize: 11
                            }
                            Text {
                                text: modelData.value
                                color: "#58A6FF"; font.family: "Courier New"; font.pixelSize: 11
                            }
                        }
                    }

                    Row {
                        width: parent.width
                        Text { width: 90; text: "Uptime"; color: "#7D8590"; font.pixelSize: 11 }
                        Text {
                            id: uptimeText
                            color: "#58A6FF"; font.family: "Courier New"; font.pixelSize: 11
                            property int s: 0
                            text: String(Math.floor(s/3600)).padStart(2,"0") + ":" +
                                  String(Math.floor((s%3600)/60)).padStart(2,"0") + ":" +
                                  String(s%60).padStart(2,"0")
                            Timer { interval:1000; running:true; repeat:true; onTriggered: uptimeText.s++ }
                        }
                    }
                }

                Rectangle { width: parent.width; height: 1; color: "#30363D" }

                // Pipeline
                Text {
                    text: "PIPELINE"
                    color: "#7D8590"
                    font.pixelSize: 11; font.bold: true; font.letterSpacing: 1.5
                }

                Column {
                    width: parent.width
                    spacing: 1
                    Repeater {
                        model: [
                            {text:"SerialPortReader",  arrow:false},
                            {text:"↓  NMEA sentence",  arrow:true },
                            {text:"TelemetryParser",   arrow:false},
                            {text:"↓  SensorFrame",    arrow:true },
                            {text:"DataBuffer<256>",   arrow:false},
                            {text:"↓  pop()",          arrow:true },
                            {text:"AnomalyDetector",   arrow:false},
                            {text:"↓  Qt signal",      arrow:true },
                            {text:"SensorModel (QML)", arrow:false},
                        ]
                        delegate: Text {
                            text: modelData.text
                            color: modelData.arrow ? "#7D8590" : "#58A6FF"
                            font.family: "Courier New"; font.pixelSize: 11
                        }
                    }
                }
            }
        }
    }

    // ── Footer ─────────────────────────────────────────────────────────────
    Rectangle {
        id: footer
        anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
        height: 44
        color: "#161B22"
        Text {
            anchors { verticalCenter: parent.verticalCenter; left: parent.left; leftMargin: 16 }
            text: "nmea-monitor-cpp v1.0.0  ·  NMEA-0183 / IEC 61162-1  ·  C++20 · Qt6  ·  Diego Velázquez  ·  github.com/dvo18"
            color: "#7D8590"; font.pixelSize: 11
        }
        Text {
            anchors { verticalCenter: parent.verticalCenter; right: parent.right; rightMargin: 16 }
            text: sensorModel.count + " channels active"
            color: "#4CFF91"; font.pixelSize: 11
        }
    }
}
