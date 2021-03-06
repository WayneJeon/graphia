/* Copyright © 2013-2020 Graphia Technologies Ltd.
 *
 * This file is part of Graphia.
 *
 * Graphia is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Graphia is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Graphia.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.7
import QtQuick.Dialogs 1.2
import QtQuick.Controls 1.5
import QtQuick.Layouts 1.3

import app.graphia 1.0

import "../../../../shared/ui/qml/Constants.js" as Constants

import "../Controls"

Item
{
    LimitConstants { id: limitConstants }

    Preferences
    {
        id: visuals;
        section: "visuals"

        property alias defaultNodeColor: nodeColorPickButton.color
        property alias defaultEdgeColor: edgeColorPickButton.color
        property alias multiElementColor: multiElementColorPickButton.color
        property alias backgroundColor: backgroundColorPickButton.color
        property alias highlightColor: highlightColorPickButton.color

        property double defaultNodeSize
        property double defaultEdgeSize
        property alias transitionTime: transitionTimeSlider.value
        property double minimumComponentRadius

        property string textFont
        property var textSize
        property int textAlignment
    }

    Component.onCompleted:
    {
        nodeSizeSlider.value = visuals.defaultNodeSize;
        edgeSizeSlider.value = visuals.defaultEdgeSize;
        minimumComponentRadiusSlider.value = visuals.minimumComponentRadius;

        delayedPreferences.enabled = true;
    }

    Timer
    {
        id: delayedPreferences

        property bool enabled: false

        interval: 250
        repeat: false

        function update()
        {
            if(!enabled)
                return;

            restart();
        }

        onTriggered:
        {
            if(!enabled)
                return;

            visuals.defaultNodeSize = nodeSizeSlider.value;
            visuals.defaultEdgeSize = edgeSizeSlider.value;
            visuals.minimumComponentRadius = minimumComponentRadiusSlider.value;
        }
    }

    RowLayout
    {
        anchors.fill: parent
        anchors.margins: Constants.margin
        spacing: Constants.spacing

        GridLayout
        {
            columns: 2
            rowSpacing: Constants.spacing
            columnSpacing: Constants.spacing

            Label
            {
                Layout.columnSpan: 2

                font.bold: true
                text: qsTr("Colours")
            }

            Label { text: qsTr("Nodes") }
            ColorPickButton { id: nodeColorPickButton }

            Label { text: qsTr("Edges") }
            ColorPickButton { id: edgeColorPickButton }

            Label { text: qsTr("Multi Elements") }
            ColorPickButton { id: multiElementColorPickButton }

            Label { text: qsTr("Background") }
            ColorPickButton { id: backgroundColorPickButton }

            Label { text: qsTr("Selection") }
            ColorPickButton { id: highlightColorPickButton }

            Label
            {
                Layout.columnSpan: 2
                Layout.topMargin: Constants.margin * 2

                font.bold: true
                text: qsTr("Sizes")
            }

            Label { text: qsTr("Nodes") }
            Slider
            {
                id: nodeSizeSlider
                minimumValue: limitConstants.minimumNodeSize
                maximumValue: limitConstants.maximumNodeSize

                onValueChanged: { delayedPreferences.update(); }
            }

            Label { text: qsTr("Edges") }
            Slider
            {
                id: edgeSizeSlider
                minimumValue: limitConstants.minimumEdgeSize
                maximumValue: limitConstants.maximumEdgeSize

                onValueChanged: { delayedPreferences.update(); }
            }

            Item { Layout.fillHeight: true }
        }

        GridLayout
        {
            columns: 2
            rowSpacing: Constants.spacing
            columnSpacing: Constants.spacing

            Label
            {
                Layout.columnSpan: 2

                font.bold: true
                text: qsTr("Text")
            }

            Label { text: qsTr("Font") }
            Button
            {
                text: visuals.textFont + " " + visuals.textSize + "pt";
                onClicked: { fontDialog.visible = true }
            }

            Label { text: qsTr("Alignment") }
            ComboBox
            {
                model:
                [
                    // Must stay synced with TextAlignment in graphrenderer.h
                    qsTr("Right"),
                    qsTr("Left"),
                    qsTr("Centre"),
                    qsTr("Top"),
                    qsTr("Bottom")
                ]
                currentIndex: visuals.textAlignment
                onCurrentIndexChanged: visuals.textAlignment = currentIndex;
            }

            Label
            {
                Layout.columnSpan: 2
                Layout.topMargin: Constants.margin * 2

                font.bold: true
                text: qsTr("Miscellaneous")
            }

            Label { text: qsTr("Transition Time") }
            Slider
            {
                id: transitionTimeSlider
                minimumValue: limitConstants.minimumTransitionTime
                maximumValue: limitConstants.maximumTransitionTime
            }

            Label { text: qsTr("Component Radius") }
            Slider
            {
                id: minimumComponentRadiusSlider
                minimumValue: limitConstants.minimumMinimumComponentRadius
                maximumValue: limitConstants.maximumMinimumComponentRadius

                onValueChanged: { delayedPreferences.update(); }
            }

            Item { Layout.fillHeight: true }
        }

        Item { Layout.fillWidth: true }
    }

    FontDialog
    {
        id: fontDialog
        title: qsTr("Please choose a font")
        currentFont: Qt.font({ family: visuals.textFont, pointSize: visuals.textSize, weight: Font.Normal })
        font: Qt.font({ family: visuals.textFont, pointSize: visuals.textSize, weight: Font.Normal })
        onAccepted:
        {
            visuals.textFont = fontDialog.font.family;
            visuals.textSize = fontDialog.font.pointSize;
        }
    }
}



