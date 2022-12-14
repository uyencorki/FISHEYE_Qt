#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QtCore>

QT_BEGIN_NAMESPACE

namespace Ui
{
class MainWindow;
}

QT_END_NAMESPACE

class MainWindow : public QMainWindow
{

    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    QString input_folder, output_folder;
    int mode_change ;
    QTimer *timer;


private slots:
    void on_spinBox_valueChanged(int arg1);
    void on_start_clicked();
    void MyTimerSlot();

    void on_Stop_Button_clicked();

    void on_choose_input_clicked();
    void on_choose_output_clicked();

    void on_Test_Button_clicked();

public slots:
    void get_run_percent(int ) ;
    void incrThreadDoneChange();


private:
    Ui::MainWindow *ui;

signals:
    void pass_dir(QString, QString, int, int );
    void stop_signal();

};
#endif // MAINWINDOW_H
