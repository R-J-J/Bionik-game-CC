#include "mainrozpoznawator.h"

const int FRAME_WIDTH = 640;
const int FRAME_HEIGHT = 480;

using namespace  cv;
using namespace aruco;


aruco::MarkerDetector MainRozpoznawator::detector() const
{
    return mDetector;
}

void MainRozpoznawator::setMinColRange(unsigned int value)
{
    minColRange = value;
}

void MainRozpoznawator::setMaxColRange(unsigned int value)
{
    maxColRange = value;
}

void MainRozpoznawator::setMinRowRange(unsigned int value)
{
    minRowRange = value;
}

void MainRozpoznawator::setMaxRowRange(unsigned int value)
{
    maxRowRange = value;
}

MainRozpoznawator::MainRozpoznawator()
    : rythm(this)
{
    /////////////////////// Camera capture setup
    this->cameraCapture.open(0);
    //set height and width of capture frame
    cameraCapture.set(CV_CAP_PROP_FRAME_WIDTH, FRAME_WIDTH);
    cameraCapture.set(CV_CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT);
    ////////////////////////////////////////////////////////////////


#ifdef DEBUG
    namedWindow(windowName, 1);
#endif
    /////////////////////// Setup for tracking boxes
    Block red_block("red_block"), green_block("green_block"),
            blue_block("blue_block"), yellow_block("yellow_block");

    this->block_types.push_back(red_block);
    this->block_types.push_back(green_block);
    this->block_types.push_back(blue_block);
    this-> block_types.push_back(yellow_block);
    ////////////////////////////////////////////////////////////////


    ////////////////////// Setup for tracking robots
    this->mDetector.setDesiredSpeed(3);
    ////////////////////////////////////////////////////////////////

    ////////////////////// Other values setup
    this->centerY = FRAME_HEIGHT/2;
    this->centerX = FRAME_WIDTH/2;

    this->minColRange = 0;
    this->minRowRange = 0;
    this->maxColRange = FRAME_WIDTH;
    this->maxRowRange = FRAME_HEIGHT;
    ////////////////////////////////////////////////////////////////

    /**
     * Wystartowanie zegara z czasem 0 powoduje jego natychmiastowe
     * włączanie kiedy tylko nie ma żadnych innych zdarzeń
     * do przetworzenia w pętli.
     */
    QObject::connect(&rythm, &QTimer::timeout, this, &MainRozpoznawator::mainWork);
    rythm.start(0);
}



void MainRozpoznawator::findBoxes(){


    Mat cameraFeed = this->cameraFrame;
    Mat threshold;
    Mat HSV;

    vector<Block> all_blocks;


    // Run through all types of blocks and find them on the image
    cvtColor(cameraFeed, HSV, COLOR_BGR2HSV);
    for(unsigned int i = 0; i < this->block_types.size(); ++i){
        inRange(HSV, this->block_types.at(i).getHSVmin(), this->block_types.at(i).getHSVmax(),
                threshold);
        morphOps(threshold);
        trackFilteredObject(&all_blocks, this->block_types.at(i), threshold, HSV,
                cameraFeed);
    }



    for (unsigned int i = 0; i < all_blocks.size(); ++i) {

        ColorBox::Color color;
        if(all_blocks.at(i).getType()=="red_block"){                //TODO: Zmienić string w klasie Block na jakiś enum

        color = ColorBox::Color::RED;

        }else if(all_blocks.at(i).getType()=="green_block"){

        color = ColorBox::Color::GREEN;

        }else if(all_blocks.at(i).getType()=="blue_block"){

        color = ColorBox::Color::BLUE;

        }else if(all_blocks.at(i).getType()=="yellow_block"){

        }
        ColorBox box;
        box.color = color;
        box.xCentimeters = all_blocks.at(i).getXPos() - centerX;
        box.yCentimeters = all_blocks.at(i).getYPos() - centerY;
        box.area = all_blocks.at(i).getArea();
        this->colorBoxesInfo.push_back(box);
    }

#ifdef DEBUG
    drawObject(all_blocks, drawFrame);
#endif


}

void MainRozpoznawator::morphOps(Mat &thresh) {

    //create structuring element that will be used to "dilate" and "erode" image.
    //the element chosen here is a 3px by 3px rectangle

    Mat erodeElement = getStructuringElement(MORPH_RECT, Size(3, 3));
    //dilate with larger element so make sure object is nicely visible
    Mat dilateElement = getStructuringElement(MORPH_RECT, Size(8, 8));

    erode(thresh, thresh, erodeElement);
    erode(thresh, thresh, erodeElement);

    dilate(thresh, thresh, dilateElement);
    dilate(thresh, thresh, dilateElement);
}



void MainRozpoznawator::trackFilteredObject(vector<Block>* blocks, Block theBlock, Mat threshold,
        Mat HSV, Mat &cameraFeed) {

    //max number of objects to be detected in frame
    const int MAX_NUM_OBJECTS = 50;
    //minimum object area
    int MIN_OBJECT_AREA = 20 * 20;

    Mat temp;
    threshold.copyTo(temp);
    //these two vectors needed for output of findContours
    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;
    //find contours of filtered image using openCV findContours function
    findContours(temp, contours, hierarchy, CV_RETR_CCOMP,
            CV_CHAIN_APPROX_SIMPLE);
    //use moments method to find our filtered object

    if (hierarchy.size() > 0) {
        int numObjects = hierarchy.size();
        //if number of objects greater than MAX_NUM_OBJECTS we have a noisy filter
        if (numObjects < MAX_NUM_OBJECTS) {
            for (int index = 0; index >= 0; index = hierarchy[index][0]) {

                Moments moment = moments((cv::Mat) contours[index]);
                double area = moment.m00;

                //if the area is less than 20 px by 20px then it is probably just noise
                //if the area is the same as the 3/2 of the image size, probably just a bad filter
                //we only want the object with the largest area so we safe a reference area each
                //iteration and compare it to the area in the next iteration.
                if (area > MIN_OBJECT_AREA) {

                    Block block;

                    block.setXPos(moment.m10 / area);
                    block.setYPos(moment.m01 / area);
                    block.setType(theBlock.getType());
                    block.setColour(theBlock.getColour());
                    block.setArea(area);

                    blocks->push_back(block);

                }

            }

        } else
            putText(cameraFeed, "TOO MUCH NOISE! ADJUST FILTER", Point(0, 50),
                    1, 2, Scalar(0, 0, 255), 2);
    }
}


void MainRozpoznawator::drawObject(vector<Block> theBlocks, Mat &frame) {

    for (unsigned int i = 0; i < theBlocks.size(); i++) {

        cv::circle(frame,
                cv::Point(theBlocks.at(i).getXPos(), theBlocks.at(i).getYPos()),
                10, cv::Scalar(0, 0, 255));
        cv::putText(frame,
                to_string(theBlocks.at(i).getXPos()) + " , "
                        + to_string(theBlocks.at(i).getYPos()),
                cv::Point(theBlocks.at(i).getXPos(),
                        theBlocks.at(i).getYPos() + 20), 1, 1,
                Scalar(0, 255, 0));
        cv::putText(frame, theBlocks.at(i).getType(),
                cv::Point(theBlocks.at(i).getXPos(),
                        theBlocks.at(i).getYPos() - 30), 1, 2,
                theBlocks.at(i).getColour());
    }
}


double calculate_angle(Marker marker) {

    cv::Point2f point1 = marker[1];
    cv::Point2f point2 = marker[2];
    //cout << point1 << "     " << point2 << endl;
    //cout << point1.y << "     " << point2.y << endl;
    double slope = (point1.x - point2.x) / (point1.y - point2.y);

    double angle_beta = atan(slope);

    if ((point1.x >= point2.x) && (point1.y < point2.y)) {			// 1 cwiartka
        return (-angle_beta);
    } else if ((point1.x >= point2.x) && (point1.y >= point2.y)) {  // 2 cwiartka
        return (-angle_beta + M_PI);
    } else if ((point1.x < point2.x) && (point1.y >= point2.y)) {   // 3 cwiartka
        return (-angle_beta + M_PI);
    } else {                                                        // 4 cwiartka
        return (-angle_beta + 2*M_PI);
    }
}

void MainRozpoznawator::findRobots() {
    //Detecting AR markers //

    vector<Marker> markers;
    this->mDetector.detect(this->cameraFrame,markers);
    double angle = 0;


    for (unsigned int i=0;i<markers.size();i++) {
        if( markers.at(i).id == this->robot1Id ){
            Robot robot;
            robot.robotId = 1;
            robot.rotationRadians = angle;
            robot.xCentimeters = markers.at(i).getCenter().x - this->centerX;
            robot.yCentimeters = markers.at(i).getCenter().y - this->centerY;
            this->robotsinfo.push_back(robot);
#ifdef DEBUG
            putText(this->drawFrame, to_string(180 * angle / M_PI),    //draw marker info and its boundaries in the image
                    cv::Point(markers[i][0].x,
                            markers[i][0].y), 1, 3,
                    Scalar(0, 0, 255),3);
            markers[i].draw(this->drawFrame,Scalar(0,0,255),2);
#endif;
        } else if( markers.at(i).id == this->robot2Id ){
            angle  = calculate_angle(markers[i]);
            Robot robot;
            robot.robotId = 2;
            robot.rotationRadians = angle;
            robot.xCentimeters = markers.at(i).getCenter().x - this->centerX;
            robot.yCentimeters = markers.at(i).getCenter().y - this->centerY;
            this->robotsinfo.push_back(robot);
#ifdef DEBUG
            putText(this->drawFrame, to_string(180 * angle / M_PI),    //draw marker info and its boundaries in the image
                    cv::Point(markers[i][0].x,
                            markers[i][0].y), 1, 3,
                    Scalar(0, 0, 255),3);
            markers[i].draw(this->drawFrame,Scalar(0,0,255),2);
#endif;
        }

    }

}

struct colourCalibCallbackData {
    Mat cameraFeed;
    unsigned int* colours_calibrated;
    fstream* file;
} ;


void colourCalibCallback(int event, int x, int y, int flags, void* userdata) {      // Funkcja callback dla kliknięcia myszki przy kalibracji kolorów


    if (event == EVENT_LBUTTONDOWN) {
        int margin = 20;
        int h_colour_tolerance = 10;
        int s_colour_tolerance = 100;
        int v_colour_tolerance = 100;

        colourCalibCallbackData* callback_data = (colourCalibCallbackData*) userdata;
        Mat cameraFeed = callback_data->cameraFeed;
        unsigned int* colours_calibrated = callback_data->colours_calibrated;
        fstream* file = callback_data->file;

#ifdef DEBUG
        cout << "Left button of the mouse is clicked - position (" << x << ", "
                << y << ")" << endl;
#endif


        // Calculate the borders of the area around the mouse click point
        int minMarginX = x - margin;
        if (minMarginX <= 0)
            minMarginX = 0;
        int minMarginY = y - margin;
        if (minMarginY <= 0)
            minMarginY = 0;
        int maxMarginX = x + margin;
        if (maxMarginX > FRAME_WIDTH)
            maxMarginX = FRAME_WIDTH;
        int maxMarginY = y + margin;
        if (maxMarginY > FRAME_HEIGHT)
            maxMarginY = FRAME_HEIGHT;


        Mat croppedFeed = cameraFeed.colRange(minMarginX, maxMarginX);
        croppedFeed = croppedFeed.rowRange(minMarginY, maxMarginY);

        typedef cv::Point3_<uint8_t> Pixel;

        int hChannelSum = 0;
        int sChannelSum = 0;
        int vChannelSum = 0;


        // Calculate mean value of base colours, by summing these values from each pixel and dividing by a number of pixels
        for (int r = 0; r < croppedFeed.rows; ++r) {
            Pixel* ptr = croppedFeed.ptr < Pixel > (r, 0);
            const Pixel* ptr_end = ptr + croppedFeed.cols;
            for (; ptr != ptr_end; ++ptr) {
                hChannelSum += ptr->x;
                sChannelSum += ptr->y;
                vChannelSum += ptr->z;
            }
        }

        hChannelSum = hChannelSum/(croppedFeed.rows * croppedFeed.cols);
        sChannelSum = sChannelSum/(croppedFeed.rows * croppedFeed.cols);
        vChannelSum = vChannelSum/(croppedFeed.rows * croppedFeed.cols);
        // End of calculating mean values

        int hChannelMin = (hChannelSum - h_colour_tolerance) > 0 ? (hChannelSum - h_colour_tolerance) : 0;
        int sChannelMin = (sChannelSum - s_colour_tolerance) > 0 ? (sChannelSum - s_colour_tolerance) : 0;
        int vChannelMin = (vChannelSum - v_colour_tolerance) > 0 ? (vChannelSum - v_colour_tolerance) : 0;

        int hChannelMax = (hChannelSum + h_colour_tolerance) < 256 ? (hChannelSum + h_colour_tolerance) : 256;

        int sChannelMax = (sChannelSum + s_colour_tolerance) < 256 ? (sChannelSum + s_colour_tolerance) : 256;
        int vChannelMax = (vChannelSum + v_colour_tolerance) < 256 ? (vChannelSum + v_colour_tolerance) : 256;


        *file << hChannelMin << endl;
        *file << sChannelMin << endl;
        *file << vChannelMin << endl;
        *file << hChannelMax << endl;
        *file << sChannelMax << endl;
        *file << vChannelMax << endl;


        cout << hChannelSum << " " << sChannelSum << " " << vChannelSum << endl;
        imshow("Cropped", croppedFeed);
        *colours_calibrated += 1;

    }
}

void MainRozpoznawator::colourCalibration() {


    Mat cameraFeed;
    Mat HSV;
    unsigned int colours_calibrated = 0;

    fstream file;
    file.open("colour_data.txt", ios::out);

    namedWindow("Calibration", 1);
    namedWindow("Cropped", 1);

    colourCalibCallbackData callback_data;


    callback_data.colours_calibrated = &colours_calibrated;
    callback_data.file = &file;

    setMouseCallback("Calibration", colourCalibCallback, &callback_data);

    while ( colours_calibrated < this->block_types.size() ) {
        this->cameraCapture.read(cameraFeed);
        flip(cameraFeed,cameraFeed, 1);
        cvtColor(cameraFeed, HSV, COLOR_BGR2HSV);
        callback_data.cameraFeed = HSV;
        imshow("Calibration", cameraFeed);
        waitKey(1);
    }


    file.close();

    // Repeat operations from the constructor to update block objects with new values
    this->block_types.clear();

    Block red_block("red_block"), green_block("green_block"),
            blue_block("blue_block"), yellow_block("yellow_block");

    this->block_types.push_back(red_block);
    this->block_types.push_back(green_block);
    this->block_types.push_back(blue_block);
    this-> block_types.push_back(yellow_block);

    destroyWindow("Calibration");
    destroyWindow("Cropped");
}

void MainRozpoznawator::findBoardPos( Mat* cameraFeed ){

    vector<Marker> markers;
    this->mDetector.detect(*cameraFeed,markers);

    Marker topLeft, topRight, bottomLeft, bottomRight;

    for (unsigned int i=0;i<markers.size();i++) {
        if( markers.at(i).id == this->topLeftBoardId ){
            topLeft = markers.at(i);
            markers[i].draw(*cameraFeed,Scalar(0,0,255),2);
        }else if( markers.at(i).id == this->topRightBoardId ){
            topRight = markers.at(i);
            markers[i].draw(*cameraFeed,Scalar(0,0,255),2);
        }else if( markers.at(i).id == this->bottomLeftBoardId ){
            bottomLeft = markers.at(i);
            markers[i].draw(*cameraFeed,Scalar(0,0,255),2);
        }else if( markers.at(i).id == this->bottomRightBoardId ){
            bottomRight = markers.at(i);
            markers[i].draw(*cameraFeed,Scalar(0,0,255),2);
        }
    }


    this->minColRange = min({topLeft.getCenter().y, topRight.getCenter().y, bottomLeft.getCenter().y, bottomRight.getCenter().y});
    this->minRowRange = min({topLeft.getCenter().x, topRight.getCenter().x, bottomLeft.getCenter().x, bottomRight.getCenter().x});
    this->maxColRange = max({topLeft.getCenter().y, topRight.getCenter().y, bottomLeft.getCenter().y, bottomRight.getCenter().y});
    this->maxRowRange = max({topLeft.getCenter().x, topRight.getCenter().x, bottomLeft.getCenter().x, bottomRight.getCenter().x});
}

void MainRozpoznawator::crop2Board(){
    this->cameraFrame = this->cameraFrame.colRange(this->minColRange,this->maxColRange);
    this->cameraFrame = this->cameraFrame.rowRange(this->minRowRange,this->maxRowRange);
}

struct BoardConfCallbackData {
    Mat* cameraFeed;
    bool* board_configured;
    MainRozpoznawator* rozpoznawator;
} ;

void boardConfCallback(int event, int x, int y, int flags, void* userdata){         // Funkcja callback dla kliknięcia myszki przy wykrywaniu planszy

    if( event == EVENT_LBUTTONDOWN ){

        BoardConfCallbackData* data = (BoardConfCallbackData*) userdata;

        bool topLeftFound = false;
        bool topRightFound = false;
        bool bottomLeftFound = false;
        bool bottomRightFound = false;
        vector<Marker> markers;
        data->rozpoznawator->detector().detect(*(data->cameraFeed),markers);

        Marker topLeft, topRight, bottomLeft, bottomRight;

        for (unsigned int i=0;i<markers.size();i++) {
            if( markers.at(i).id == data->rozpoznawator->topLeftBoardId ){
                topLeft = markers.at(i);
                topLeftFound = true;
            }else if( markers.at(i).id == data->rozpoznawator->topRightBoardId ){
                topRight = markers.at(i);
                topRightFound = true;
            }else if( markers.at(i).id == data->rozpoznawator->bottomLeftBoardId ){
                bottomLeft = markers.at(i);
                bottomLeftFound = true;
            }else if( markers.at(i).id == data->rozpoznawator->bottomRightBoardId ){
                bottomRight = markers.at(i);
                bottomRightFound = true;
            }
        }

        if( topLeftFound && topRightFound && bottomLeftFound && bottomRightFound ){
            data->rozpoznawator->setMinRowRange( min({topLeft.getCenter().y, topRight.getCenter().y, bottomLeft.getCenter().y, bottomRight.getCenter().y}) );
            data->rozpoznawator->setMinColRange( min({topLeft.getCenter().x, topRight.getCenter().x, bottomLeft.getCenter().x, bottomRight.getCenter().x}) );
            data->rozpoznawator->setMaxRowRange( max({topLeft.getCenter().y, topRight.getCenter().y, bottomLeft.getCenter().y, bottomRight.getCenter().y}) );
            data->rozpoznawator->setMaxColRange( max({topLeft.getCenter().x, topRight.getCenter().x, bottomLeft.getCenter().x, bottomRight.getCenter().x}) );
            topLeft.draw(*(data->cameraFeed),Scalar(0,0,255),2);
            topRight.draw(*(data->cameraFeed),Scalar(0,0,255),2);
            bottomLeft.draw(*(data->cameraFeed),Scalar(0,0,255),2);
            bottomRight.draw(*(data->cameraFeed),Scalar(0,0,255),2);
            *(data->board_configured) = true;
        }

    }
}

void MainRozpoznawator::drawCross( Mat* frame ){
    line(*frame,Point(this->centerX - 10, this->centerY), Point(this->centerX + 10, this->centerY), Scalar(0,0,255), 2);
    line(*frame,Point(this->centerX , this->centerY- 10), Point(this->centerX , this->centerY + 10), Scalar(0,0,255), 2);
}

void MainRozpoznawator::boardConfiguration(){

    Mat cameraFeed;
    Mat drawFrame;
    MarkerDetector markerDetector;
    bool board_configured = false;
    BoardConfCallbackData callback_data;

    callback_data.board_configured = &board_configured;
    callback_data.rozpoznawator = this;

    vector<Marker> Markers;
    namedWindow("Board_Config", 1);


    setMouseCallback("Board_Config", boardConfCallback , &callback_data);

    while ( !board_configured ) {
        this->cameraCapture.read(cameraFeed);
        this->cameraCapture.read(drawFrame);
        flip(cameraFeed,cameraFeed, 1);
        flip(drawFrame,drawFrame, 1);
        callback_data.cameraFeed = &cameraFeed;

        Markers.clear();
        markerDetector.detect(cameraFeed,Markers);

        for (unsigned int i=0;i<Markers.size();i++) {
            if( Markers.at(i).id == this->topLeftBoardId ){
               Markers[i].draw(drawFrame,Scalar(0,0,255),2);
            }else if( Markers.at(i).id == this->topRightBoardId ){
               Markers[i].draw(drawFrame,Scalar(0,0,255),2);
            }else if( Markers.at(i).id == this->bottomLeftBoardId ){
               Markers[i].draw(drawFrame,Scalar(0,0,255),2);
            }else if( Markers.at(i).id == this->bottomRightBoardId ){
               Markers[i].draw(drawFrame,Scalar(0,0,255),2);
            }

        }
        this->drawCross(&drawFrame);
        imshow("Board_Config", drawFrame);
        waitKey(1);
    }
    imshow("Board_Config", cameraFeed);
    waitKey(10000);
    destroyWindow("Board_Config");


}

void MainRozpoznawator::mainWork()
{
    int sleepTime = 1 + qrand()%90;
    QThread::msleep(sleepTime);

    this->robotsinfo.clear();
    this->colorBoxesInfo.clear();

    this->cameraCapture.read(this->cameraFrame);
    flip(this->cameraFrame, this->cameraFrame, 1);

    crop2Board();
#ifdef DEBUG
    this->drawFrame = this->cameraFrame;
#endif

    findBoxes();
    findRobots();

#ifdef DEBUG
    imshow(windowName, this->drawFrame);
#endif



   emit gameState(this->robotsinfo, this->colorBoxesInfo);
}


