#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // thread
//    qsrand(0);
//    auto threadTest = std::function<void ()> ([&]()
//    {
//    if(QCoreApplication::instance()->thread() == QThread::currentThread())
//    {
//        qDebug() << "UI Thread in use";
//    }
//    else
//    {
//         qDebug() << "Worker thread in use";
//    }

//    for (int var = 0; var < INT_MAX; ++var)
//      {
//              int r = qrand() % 100;
//              QThread::msleep(r);
//              qDebug() << "[Worker Thread " << QThread::currentThreadId() << "] " << r;
//      }
//      });

//      QThread *thread = QThread::create(threadTest);
//      thread->start();
//      int var = 0;
//      std::function<void ()> timerTest;

//      timerTest = [&]()
//      {
//          int r = qrand() % 100;
//          qDebug() << "[UI Thread " << QThread::currentThreadId() << "] " << r;
//          ++var;
//          if (var < INT_MAX)
//              QTimer::singleShot(r, timerTest);
//      };

//      int r = qrand() % 100;
//      QTimer::singleShot(r, timerTest);


    MainWindow w;
    w.show();
    return a.exec();
}
