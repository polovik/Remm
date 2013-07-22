import QtQuick 2.0

Rectangle {
    width: 500
    height: 500

    Item {
        id: item1
        x: 0
        y: 0
        width: 320
        height: 240
    }

    Item {
        id: item2
        x: 320
        y: 241
        width: 182
        height: 160
    }

    Canvas {
        id: outboardDisplayCanvas
        x: 0
        y: 241
        width: 279
        height: 238
        onPaint: {
            var ctx = outboardDisplayCanvas.getContext('2d')

            var ellipse_width = 200
            var ellipse_height = 200
            var tangageAngle = 8  //  in range [-180, 180]
            var tangageAnglesRange = 20   //  display +-20 degrees
            var tangageAngleStep = ellipse_height * 0.8 / (tangageAnglesRange * 2) // tangages's dashes occupy only 50% of height

            var heelAngle = 5 * Math.PI / 180
            ctx.translate(ellipse_width / 2, ellipse_height / 2)
            ctx.rotate(heelAngle)

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
}
