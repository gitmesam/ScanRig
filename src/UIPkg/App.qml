import QtQuick 2.0
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.14
import "Components"

ApplicationWindow {
    title: "ScanRig App"
    visible: true
    width: 640
    height: 480

    ColumnLayout {
        Flow {
            spacing: 2

            CButton {
                text: "Stop"
                onClicked: pageLoader.source = "Views/Preview.qml"
            }

            CButton {
                text: "Start"
                onClicked: pageLoader.source = "Views/Capture.qml"
            }
        }

        Flow {
            Loader {
                id: pageLoader
                source: "Views/Preview.qml"
            }

            CCameraSettings {
            }
        }
    }

}
