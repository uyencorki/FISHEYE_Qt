#ifndef WORKER_H
#define WORKER_H

#include "mainwindow.h"

#include <QThread>

#include <QMutex>
#include <QWaitCondition>
#include <QFutureSynchronizer>


class Worker : public QThread
{
  Q_OBJECT

public:
  explicit Worker(int);

private:
  int threads = 0;

  QAtomicInt stopped = 0;
  QAtomicInt pause = 0;
  bool exit = false;
  QMutex sync;
  QWaitCondition cond;
  QFutureSynchronizer<void> pool;

  void run();
  void task_1();
  void task_2();
  void task_3();
  void task_4();
  void task_5();
  void Measure_time();


private slots:
  void stopThreads();
  void pauseThreads();
  void resumeThreads();
  void receive_dir(QString, QString, int, int);


signals:
  void incrThreadDone();
  void run_percent (int);

};


#endif // WORKER_H
