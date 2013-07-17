import QtQuick 2.1
import QtWebKit 3.0
import QtQuick.Controls 1.0
import QtMultimedia 5.0

Rectangle {
    width: 700
    height: 500

    WebView {
        id: navigationView
        x: 44
        y: 259
        url: "google_maps.html"
//        url: "http://www.ya.ru"
        width: 272
        height: 241
        objectName: "navigationView"
        Keys.onPressed:{ console.log("Key pressed"); }
//        focus: false
/*        onNavigationRequested: {
            console.log("url =", request.url, "navigationType =", request.navigationType)
            switch (request.navigationType)
            {
            case WebView.LinkClickedNavigation:
            case WebView.FormSubmittedNavigation:
            case WebView.BackForwardNavigation:
            case WebView.ReloadNavigation:
            case WebView.FormResubmittedNavigation:
            case WebView.OtherNavigation:
                    request.action = WebView.AcceptRequest
                    return
            }
//            request.action = WebView.IgnoreRequest
            request.action = WebView.AcceptRequest
        }*/
/*        MouseArea {
            id: mousearea1
            x: 145
            y: 144
            width: 180
            height: 107
            onClicked: {
                navigationView.flick(100,100)
            }
        }
        */
    }

    Canvas {
        id: outboardDisplayCanvas
        x: 379
        y: 19
        width: 279
        height: 238
        property int tangageAngle: 8 //  in range [-180, 180]
        property int heelAngle: 5
        onPaint: {
            var ctx = outboardDisplayCanvas.getContext('2d')

            var ellipse_width = 200
            var ellipse_height = 200
            var tangageAnglesRange = 20   //  display +-20 degrees
            var tangageAngleStep = ellipse_height * 0.8 / (tangageAnglesRange * 2) // tangages's dashes occupy only 50% of height

            ctx.translate(ellipse_width / 2, ellipse_height / 2)
            ctx.rotate(heelAngle * Math.PI / 180)

            //  Prepare to draw background
            var groundLevel = tangageAngle * tangageAngleStep
            var groundLevelAngle = Math.asin(groundLevel / (ellipse_width / 2))
            var drawOnlySky = false
            var drawOnlyEarth = false
            if (groundLevel >= ellipse_width / 2)
                drawOnlySky = true
            else if (groundLevel <= -ellipse_width / 2)
                drawOnlyEarth = true
//            console.log("drawOnlyEarth = ", drawOnlyEarth, "drawOnlySky = ", drawOnlySky)

            //  Draw sky
            if (!drawOnlyEarth) {
                ctx.fillStyle = "#1184CF"
                ctx.beginPath()
                if (drawOnlySky == true)
                    ctx.arc(0, 0, ellipse_width / 2, 0,  2 * Math.PI, false);
                else
                    ctx.arc(0, 0, ellipse_width / 2, Math.PI - groundLevelAngle,  groundLevelAngle, false);
                ctx.fill()
            }
            //  Draw earth
            if (!drawOnlySky) {
                ctx.fillStyle = "#794716"
                ctx.beginPath()
                if (drawOnlyEarth)
                    ctx.arc(0, 0, ellipse_width / 2, 0,  2 * Math.PI, true)
                else
                    ctx.arc(0, 0, ellipse_width / 2, Math.PI - groundLevelAngle,  groundLevelAngle, true)
                ctx.fill()
            }

            //  Draw lines of tangage
            ctx.lineWidth = 1
            ctx.strokeStyle = "#FFFFFF"
            ctx.textBaseline = "middle"
            ctx.textAlign = "right"
            for (var ang = tangageAngle - tangageAnglesRange; ang < tangageAngle + tangageAnglesRange; ang += 0.5) {
                if ((ang % 10) == 0) {
                    var dash_width = 70
                    ctx.beginPath()
                    var posY = -(ang - tangageAngle) * tangageAngleStep
                    ctx.moveTo(-dash_width / 2, posY)
                    ctx.lineTo(dash_width / 2, posY)
                    var textWidth = ctx.measureText(ang).width
                    ctx.text(ang, dash_width / 2, posY)
                    ctx.text(ang, -dash_width / 2 - textWidth, posY)
                    ctx.stroke()
                } else if ((ang % 5) == 0) {
                    var dash_width = 30
                    ctx.beginPath()
                    var posY = -(ang - tangageAngle) * tangageAngleStep
                    ctx.moveTo(-dash_width / 2, posY)
                    ctx.lineTo(dash_width / 2, posY)
                    ctx.stroke()
                } else if ((ang % 2.5) == 0) {
                    var dash_width = 12
                    ctx.beginPath()
                    var posY = -(ang - tangageAngle) * tangageAngleStep
                    ctx.moveTo(-dash_width / 2, posY)
                    ctx.lineTo(dash_width / 2, posY)
                    ctx.stroke()
                }
            }

            //  Draw current heel arrow
            ctx.fillStyle = "#FFFFFF"
            var heelArrowHeight = 14
            var heelArrowPointer = -(ellipse_width / 2)* 0.9
            ctx.beginPath()
            ctx.moveTo(0, heelArrowPointer)
            ctx.lineTo(heelArrowHeight, heelArrowPointer + heelArrowHeight)
            ctx.lineTo(-heelArrowHeight, heelArrowPointer + heelArrowHeight)
            ctx.closePath()
            ctx.fill()

            ctx.resetTransform()
            ctx.translate(ellipse_width / 2, ellipse_height / 2)

            //  Draw heel dashes
            ctx.fillStyle = "#FFFFFF"
            ctx.beginPath()
            ctx.moveTo(0, heelArrowPointer)
            ctx.lineTo(heelArrowHeight / 3, heelArrowPointer - heelArrowHeight / 1.5)
            ctx.lineTo(-heelArrowHeight / 3, heelArrowPointer - heelArrowHeight / 1.5)
            ctx.closePath()
            ctx.fill()
            ctx.lineWidth = 1
            ctx.strokeStyle = "#FFFFFF"
            var heelDashLength = 7
//            var heelDashRadius = ellipse_width / 2
            for (var ang = -50; ang <= 50; ang += 10) {
                var posX1 = heelArrowPointer * Math.sin(Math.PI / 180 * ang)
                var posY1 = heelArrowPointer * Math.cos(Math.PI / 180 * ang)
                var posX2 = (heelArrowPointer - heelDashLength) * Math.sin(Math.PI / 180 * ang)
                var posY2 = (heelArrowPointer - heelDashLength) * Math.cos(Math.PI / 180 * ang)
                ctx.beginPath()
                ctx.moveTo(posX1, posY1)
                ctx.lineTo(posX2, posY2)
                ctx.stroke()
            }

            ctx.resetTransform()

            //  Draw wings
            var wings_width = 50
            var wings_height = 10
            var left_space = (ellipse_width / 2 - wings_width) / 2
            var right_space = ellipse_width - left_space
            ctx.fillStyle = "#000000"
            ctx.lineWidth = 2
            ctx.strokeStyle = "#FFFFFF"
            ctx.beginPath()
            ctx.moveTo(left_space, ellipse_height / 2)
            ctx.lineTo(left_space + wings_width, ellipse_height / 2)
            ctx.lineTo(left_space + wings_width, ellipse_height / 2 + wings_height * 2)
            ctx.lineTo(left_space + wings_width - wings_height, ellipse_height / 2 + wings_height * 2)
            ctx.lineTo(left_space + wings_width - wings_height, ellipse_height / 2 + wings_height)
            ctx.lineTo(left_space, ellipse_height / 2 + wings_height)
            ctx.lineTo(left_space, ellipse_height / 2)
            ctx.fill()
            ctx.stroke()

            ctx.beginPath()
            ctx.moveTo(right_space, ellipse_height / 2)
            ctx.lineTo(right_space - wings_width, ellipse_height / 2)
            ctx.lineTo(right_space - wings_width, ellipse_height / 2 + wings_height * 2)
            ctx.lineTo(right_space - wings_width + wings_height, ellipse_height / 2 + wings_height * 2)
            ctx.lineTo(right_space - wings_width + wings_height, ellipse_height / 2 + wings_height)
            ctx.lineTo(right_space, ellipse_height / 2 + wings_height)
            ctx.lineTo(right_space, ellipse_height / 2)
            ctx.fill()
            ctx.stroke()

//            var ctx3 = outboardDisplayCanvas.getContext('3d')
        }
    }

    Canvas {
        id: heightCanvas
        x: 316
        y: 279
        width: 70
        height: 200
        property int currentHeight: 323
        onPaint: {
            var ctx = heightCanvas.getContext('2d')
            var areaWidth = width * 0.7

            //  Draw background
            ctx.fillStyle = "#636163"
            ctx.beginPath()
            ctx.rect(0, 0, areaWidth, height)
            ctx.fill()

            //  Draw height dashes
            ctx.translate(0, height / 2)
            var metreRange = 10 //  Display heights in range +-10metre
            var metreStep = height / (metreRange * 2)
            var heightDashWidth = width * 0.1
            ctx.strokeStyle = "#FFFFFF"
            for (var h = currentHeight - metreRange; h < currentHeight + metreRange; h++) {
                if (h % 2 == 0) {
                    ctx.lineWidth = 2
                    ctx.beginPath()
                    var posY = -(h - currentHeight) * metreStep
                    ctx.moveTo(0, posY)
                    ctx.lineTo(heightDashWidth, posY)
                    ctx.stroke()
                }
                if (h % 4 == 0) {
                    ctx.lineWidth = 1
                    ctx.beginPath()
                    var posY = -(h - currentHeight) * metreStep
                    var textWidth = ctx.measureText(h).width
                    ctx.text(h, heightDashWidth + 5, posY)
                    ctx.stroke()
                }
            }

            //  Draw current height
            ctx.fillStyle = "#000000"
            ctx.lineWidth = 2
            ctx.strokeStyle = "#FFFFFF"
            var heightArrowWidth = width * 0.1
            var heightArrowHeight = height * 0.1
            ctx.beginPath()
            ctx.moveTo(heightDashWidth, 0)
            ctx.lineTo(heightDashWidth + heightArrowWidth, heightArrowWidth)
            ctx.lineTo(heightDashWidth + heightArrowWidth, heightArrowHeight)
            ctx.lineTo(width - ctx.lineWidth, heightArrowHeight)
            ctx.lineTo(width - ctx.lineWidth, -heightArrowHeight)
            ctx.lineTo(heightDashWidth + heightArrowWidth, -heightArrowHeight)
            ctx.lineTo(heightDashWidth + heightArrowWidth, -heightArrowWidth)
            ctx.lineTo(heightDashWidth, 0)
            ctx.fill()
            ctx.stroke()

            ctx.lineWidth = 1
            ctx.strokeStyle = "#FFFFFF"
            ctx.beginPath()
            var textWidth = ctx.measureText(currentHeight).width
            ctx.text(currentHeight, heightDashWidth + heightArrowWidth + 10, 0)
            ctx.stroke()

            ctx.resetTransform()
        }
    }

    Canvas {
        id: compassCanvas
        x: 386
        y: 358
        width: 279
        height: 81
        property int curDirection: 45
        onPaint: {
            var ctx = compassCanvas.getContext('2d')
            var directionArrowHeight = height * 0.1
            var compassAnglesRange = 70 // +- 70

            //  Draw background
            ctx.fillStyle = "#000000"
            ctx.beginPath()
            ctx.rect(0, 0, width, height)
            ctx.fill()

            ctx.fillStyle = "#636163"
            ctx.beginPath()
            var arcHeight = height - directionArrowHeight
            var radious = (Math.pow(width / 2, 2) + Math.pow(arcHeight, 2)) / (2 * arcHeight)
            var ang = Math.asin((width / 2) / radious)
            ctx.arc(width / 2, directionArrowHeight + radious, radious, ang + Math.PI / 2, Math.PI / 2 - ang, false)
            ctx.closePath()
            ctx.fill()

            //  Draw current arrow
            ctx.lineWidth = 3
            ctx.strokeStyle = "#FFFFFF"
            ctx.beginPath()
            ctx.moveTo(width / 2, directionArrowHeight)
            ctx.lineTo(width / 2 - directionArrowHeight / 2, 0)
            ctx.lineTo(width / 2 + directionArrowHeight / 2, 0)
            ctx.closePath()
            ctx.stroke()

            //  Draw compass dashes
            ctx.translate(width / 2, directionArrowHeight + radious)
            var dirAnglesStep = ang * 180 / Math.PI / compassAnglesRange
//            console.log(ang, dirAnglesStep)
            for (var ang = curDirection - compassAnglesRange; ang < curDirection + compassAnglesRange; ang++) {
                if (ang % 10 == 0) {
                    var dash_width = 10
                    var radians = Math.PI - (ang - curDirection) * dirAnglesStep * Math.PI / 180
                    ctx.lineWidth = 2
                    ctx.strokeStyle = "#FFFFFF"
                    ctx.beginPath()
                    ctx.moveTo((radious - dash_width) * Math.sin(radians), (radious - dash_width) * Math.cos(radians))
                    ctx.lineTo(radious * Math.sin(radians), radious * Math.cos(radians))
                    ctx.stroke()

                    ctx.translate((radious - dash_width - 14) * Math.sin(radians), (radious - dash_width - 14) * Math.cos(radians))
                    ctx.rotate(-(Math.PI + radians))
                    var textAng = ang
                    if (ang < 0)
                        textAng = 360 + ang
                    else if (ang >= 360)
                        textAng = ang - 360
                    textAng /= 10
                    if (textAng % 9 == 0)
                        ctx.strokeStyle = "#FF0000"
                    else
                        ctx.strokeStyle = "#FFFFFF"
                    if (textAng == 0)
                        textAng = "N"
                    else if (textAng == 9)
                        textAng = "E"
                    else if (textAng == 18)
                        textAng = "S"
                    else if (textAng == 27)
                        textAng = "W"

                    var textWidth = ctx.measureText(textAng).width
                    ctx.lineWidth = 1
                    ctx.beginPath()
                    ctx.text(textAng, -textWidth / 2, 0)
                    ctx.stroke()

                    ctx.resetTransform()
                    ctx.translate(width / 2, directionArrowHeight + radious)
                }
                if (ang % 5 == 0) {
                    var dash_width = 3
                    var radians = Math.PI - (ang - curDirection) * dirAnglesStep * Math.PI / 180
                    ctx.lineWidth = 2
                    ctx.strokeStyle = "#FFFFFF"
                    ctx.beginPath()
                    ctx.moveTo((radious - dash_width) * Math.sin(radians), (radious - dash_width) * Math.cos(radians))
                    ctx.lineTo(radious * Math.sin(radians), radious * Math.cos(radians))
                    ctx.stroke()
                }
            }

            ctx.resetTransform()
        }
    }

    Slider {
        id: itemSlider1
        x: 406
        y: 312
        width: 212
        height: 22
        minimumValue: -200
        maximumValue: 200
        stepSize: 1
        tickmarksEnabled: true
        onValueChanged: {
            //print(value)
            outboardDisplayCanvas.heelAngle = value
            outboardDisplayCanvas.requestPaint()
        }

    }

    VideoOutput {
        id: cameraCanvas
        x: 44
        y: 19
        width: 320
        height: 240
        source: sourceCamera
    }
}
