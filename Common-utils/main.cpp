#include <QtGui>
#include <QApplication>
#include "joypick.h"

//#ifdef SDL_WIN
#undef main
//#endif

#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Joypick w;
    w.show();
    return a.exec();
}
