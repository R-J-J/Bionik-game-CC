#ifndef ROBOTPOSITION_H
#define ROBOTPOSITION_H

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/video.hpp>
#include <opencv2/videoio.hpp>





struct Robot
{
    unsigned robotId;
    long xCentimeters;
    long yCentimeters;
    float rotationRadians;

    bool operator==(unsigned robotId)
    {
        return this->robotId == robotId;
    }
};

struct ColorBox
{
    enum Color {RED, GREEN, BLUE} color;
    signed long xCentimeters;
    signed long yCentimeters;
    double area;
};

struct RobotCommands
{
    unsigned robotId;
    double xCentimetersPerSecond;
    double yCentimetersPerSecond;
    double zRadiansPerSecond;
};

struct BoardIndicator
{
    double xCentimetersPerSecond;
    double yCentimetersPerSecond;
};

struct RobotGoal
{
    double xCentimetersPerSecond;
    double yCentimetersPerSecond;
    unsigned goalId;
};

/**
 * Aby móc wysyłać sygnałami klasy, które nie są dostarczone przez Qt,
 * należy je zadeklarować poniżej i zarejestrować w funkcji "registerMetaTypes".
 */
#include <QObject>
Q_DECLARE_METATYPE(ColorBox)
Q_DECLARE_METATYPE(Robot)
Q_DECLARE_METATYPE(RobotCommands)
Q_DECLARE_METATYPE(BoardIndicator)
Q_DECLARE_METATYPE(RobotGoal)
Q_DECLARE_METATYPE(std::vector<ColorBox>)
Q_DECLARE_METATYPE(std::vector<Robot>)
Q_DECLARE_METATYPE(std::vector<RobotCommands>)
Q_DECLARE_METATYPE(std::vector<BoardIndicator>)
Q_DECLARE_METATYPE(std::vector<RobotGoal>)
Q_DECLARE_METATYPE(cv::Mat)
Q_DECLARE_METATYPE(std::vector<cv::Point2i>)

void registerMetaTypes();

#endif // ROBOTPOSITION_H
