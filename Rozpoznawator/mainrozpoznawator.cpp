#include "mainrozpoznawator.h"

const int FRAME_WIDTH = 1280;
const int FRAME_HEIGHT = 720;

MainRozpoznawator::MainRozpoznawator()
    : rythm(this), robotPositionX(30)
{
    cameraCapture.open(0);
    cameraCapture.set(CV_CAP_PROP_FRAME_WIDTH, FRAME_WIDTH);
    cameraCapture.set(CV_CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT);

    /**
     * Wystartowanie zegara z czasem 0 powoduje jego natychmiastowe
     * włączanie kiedy tylko nie ma żadnych innych zdarzeń
     * do przetworzenia w pętli.
     */
    QObject::connect(&rythm, &QTimer::timeout, this, &MainRozpoznawator::mainWork);
    rythm.start(0);
}

void MainRozpoznawator::updateRobotPosition()
{
    RobotPosition robotPosition = {robotPositionX, 40, 2.3};
    emit robotPositionUpdate(robotPosition);

    robotPositionX+= 1;
}

void MainRozpoznawator::updateBoxesPositions()
{
    std::vector<ColorBoxPosition> boxesPositionVector;
    for (int i = 0; i < 10; i++)
    {
        if (i%2)
        {
            ColorBoxPosition colorBoxPosition = {i+10, i*2, ColorBoxPosition::BLUE};
            boxesPositionVector.push_back(colorBoxPosition);
        }
        else
        {
            ColorBoxPosition colorBoxPosition = {2*i+50, i+9, ColorBoxPosition::GREEN};
            boxesPositionVector.push_back(colorBoxPosition);
        }
    }
    emit boxesPositionUpdate(boxesPositionVector);
}

void MainRozpoznawator::mainWork()
{
    int sleepTime = 100 + qrand()%900;
    QThread::msleep(sleepTime);

    updateRobotPosition();
    updateBoxesPositions();
}

