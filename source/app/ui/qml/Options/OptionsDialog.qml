import QtQuick 2.7
import QtQuick.Window 2.2
import QtQuick.Controls 1.5
import QtQuick.Dialogs 1.2
import QtQuick.Layouts 1.3

import "../../../../shared/ui/qml/Constants.js" as Constants

Window
{
    id: optionsWindow

    title: qsTr("Options")
    flags: Qt.Window|Qt.Dialog
    width: 720
    height: 480
    minimumWidth: 720
    minimumHeight: 480

    property bool enabled: true

    ColumnLayout
    {
        id: column
        anchors.fill: parent
        anchors.margins: Constants.margin

        TabView
        {
            id: tabView
            Layout.fillWidth: true
            Layout.fillHeight: true

            enabled: optionsWindow.enabled

            Tab
            {
                title: qsTr("Appearance")
                OptionsAppearance {}
            }

            Tab
            {
                title: qsTr("Misc")
                OptionsMisc {}
            }
        }

        Button
        {
            text: qsTr("Close")
            Layout.alignment: Qt.AlignRight
            onClicked: optionsWindow.close()
        }
    }
}

