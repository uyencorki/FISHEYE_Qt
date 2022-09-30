#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QApplication>
#include <iostream>
#include "mainwindow.h"
#include <QTimer>
#include "ui_mainwindow.h"
#include <QDir>
#include <QDebug>
#include <QElapsedTimer>
#include <QThread>
#include <QDateTime>
#include <QPlainTextEdit>
#include <QDirIterator>
#include <QDir>
#include <string>               // for strings
#include <iomanip>              // for controlling float print precision
#include <sstream>              // string to number conversion
#include <dirent.h>
#include <QtCore>

#include <opencv2/core.hpp>     // Basic OpenCV structures (cv::Mat, Scalar)
#include <opencv2/imgproc.hpp>  // Gaussian Blur
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>  // OpenCV window I/O
#include <opencv2/calib3d/calib3d.hpp>

#include <QFileDialog>
#include <QTreeView>

#include "worker.h"

const double PI = 3.1415;
using namespace cv;
using namespace std;

#define mode 1
#define old_code 0

bool stop_dis = false;
int start = 0;
QElapsedTimer timer_1;

int print_time = 0;
bool default_dir = true;

int process_time = 0 ;
bool done = false;

vector<QString> load_img;
QString load_img_file[1000000];
int file_count_M = 0;


int  buildMap_2_pano (Mat &map_x, Mat &map_y, int Ws, int Hs, int Wd, int Hd )
{
    // Ws image with, Hs image height
    // Wd new image with, Hd new image height

    float filed_of_view = 180;
    float hfovd, vfovd ;

    float Pi = 3.14159;
    float vfov,hfov;
    float vstart, hstart;
    float xmax, xmin, xscale, xoff,     zmax, zmin, zscale, zoff;

    vfovd   = filed_of_view;
    vfov    = (vfovd / 180.0) * Pi;
    vstart  = ((180.0 - vfovd) / 180.00) * Pi / 2.0;

    xmax    = sin(Pi / 2.0) * cos(vstart);
    xmin    = sin(Pi / 2.0) * cos(vstart + vfov);
    xscale  = xmax - xmin ;
    xoff    = xscale / 2.0;


    // need to scale to changed range from our
    // smaller cirlce traced by the fov


    hfovd   = filed_of_view;
    hfov    = (hfovd / 180.0) * Pi;
    hstart  = ((180.0 - hfovd) / 180.00) * Pi / 2.0;

    zmax    = cos(hstart);
    zmin    = cos(hfov + hstart);
    zscale  = zmax - zmin;
    zoff    = zscale / 2.0;

    cout << "vstart " << vstart << "  hstart    " << hstart << endl;
    cout << "vfov = " << vfov << "; hfov = " << hfov << endl;


    cout << "xmax = " << xmax << "; xmin = " << xmin << "; xscale = "
         << xscale << "; xoff = " << xoff   <<  endl;

    cout << "zmax = " << zmax <<"; zmin = " << zmin <<"; zscale = "<<zscale <<
            "; zoff = " << zoff << endl;

    Mat dst, out_dst;
    float phi, theta, xp, zp, xS, yS;
    cout << "Hd = " << Hd << "   Wd  = " << Wd << endl;

    for (int y = 0; y < Hd; y ++ )
    {
        for (int x = 0; x < Wd; x ++)
        {
            phi     = vstart + ( vfov * ( float(x) / float(Wd) ) );
            theta   = hstart + ( hfov * ( float(y) / float(Hd) ) );

            xp      = ((sin(theta) * cos(phi)) + xoff) / zscale;
            zp      = ((cos(theta)) + zoff) / zscale ;

            xS      = Ws - (xp * Ws);
            yS      = Hs - (zp * Hs);

            map_x.at<float>(x,y) = int(xS);
            map_y.at<float>(x,y) = int(yS);
        }
    }

//    cout << "  map_x.at<double>(x,y)" <<   map_x.at<double>(10,10) << endl;
//    remap( src_img, dst, map_x, map_y, INTER_LINEAR, BORDER_CONSTANT, Scalar(0, 0, 0) );

    return 0;
}


int buildMap_2_img ( Mat &map_x, Mat &map_y, int R, float Cfx, float Cfy , int new_hei, int new_wid)
{
    float Pi = 3.14159;
    float r, a, theta;
    int Xf,Yf;

    Mat src;
    src = Mat::zeros(new_hei , new_wid , CV_8UC3);
    Vec3b color;

    for (int Ye = 0; Ye < new_hei; Ye ++ )
    {
        for (int Xe = 0; Xe < new_wid; Xe ++)
        {
            r = (static_cast<float>(Ye)/static_cast<float> (new_hei)) *static_cast<float>(R) ;
            a = static_cast<float>(Xe)/static_cast<float>(new_wid);
            theta = a*2*Pi;

            Xf = int (Cfx+r*sin(theta));
            Yf = int (Cfy+r*cos(theta));

            map_x.at<float>(Ye,Xe) = int(Xf);
            map_y.at<float>(Ye,Xe) = int(Yf);
        }
    }




    return 0;
}


int findCorrespondingFisheyePoint(Mat &map_x, Mat &map_y, double He, double We,
                                      double Hf, double Wf, double FOV)
{
//       Point2f fisheyePoint;
       float theta, phi, r;
       Point3f sphericalPoint;
       Mat new_img ;

       for (int Xe = 0; Xe <Wf; Xe++)
            {
            for (int Ye = 0; Ye <Hf; Ye++)
                {
                   theta = CV_PI * (Xe / ( (float) We ) - 0.5);
                   phi   = CV_PI * (Ye / ( (float) He ) - 0.5);

                   sphericalPoint.x = cos(phi) * sin(theta);
                   sphericalPoint.y = cos(phi) * cos(theta);
                   sphericalPoint.z = sin(phi);

                   theta = atan2(sphericalPoint.z, sphericalPoint.x);
                   phi   = atan2(sqrt(pow(sphericalPoint.x,2) + pow(sphericalPoint.z,2)), sphericalPoint.y);
                   r     = ( (float) We ) * phi / FOV;

                   map_x.at<float>(Ye, Xe) = (int) ( 0.5 * ( (float) We ) + r * cos(theta) );
                   map_y.at<float>(Ye, Xe) = (int) ( 0.5 * ( (float) He ) + r * sin(theta) );
                 }
               }

        return 0;
//       return fisheyePoint;

}

int Proccess (QString input_dir, QString output_dir, int mode_change )
{
    Mat image, out_Img;
    Mat dst_flip;

#if mode
#if old_code
    image = imread ("D:/HUU_UYEN/Software/KIRARI__NINJA_DAITRON"
                    "/FISHEYE_CPP/FISHEYE_SOFTWARE/data/img23.jpg");
    cout <<"with = " << image.cols << "height = " << image.rows <<  endl;


    int Ws, Hs;
    Ws =  image.cols;
    Hs =  image.rows;

    int Wd, Hd;
    Wd =  Ws; // * (4.0 / 3.0);
    Hd =  Hs;
    if (Ws > 1920)
        resize(image, out_Img, cv::Size(), 0.3, 0.3);
    else
        resize(image, out_Img, cv::Size(), 1, 1);


    imshow ("Original Image", out_Img );

    Mat map_x = Mat::zeros(Wd, Hd,  CV_32F);
    Mat map_y = Mat::zeros(Wd, Hd,  CV_32F);

    buildMap_2_pano (map_x, map_y, Ws, Hs, Wd, Hd);

    remap( image, out_Img, map_x, map_y, INTER_LINEAR, BORDER_CONSTANT, Scalar(0, 0, 0) );
    rotate(out_Img, out_Img, ROTATE_90_CLOCKWISE);

    flip(out_Img, dst_flip, 1);
    imwrite ("out_img.jpg",dst_flip);


    if (Wd > 1920)
        resize(dst_flip, dst_flip, cv::Size(), 0.5, 0.5);

    imshow ("dst_flip", dst_flip);
#else
        // decale parameter
       int Hf, Wf;       //Height, width and FOV for the input image (=fisheyeImage)
       double FOV = PI;  //FOV in radian
       int He, We;
       Mat fisheyeImage,  cropped_image ;
       Mat convert_img;

       // process directory string
       input_dir = input_dir.replace("\\", "/");
       output_dir = output_dir.replace("\\", "/");
       String in_folder  = input_dir.toStdString();
       String out_folder = output_dir.toStdString();
       in_folder = in_folder + "/*.jpg";

       // read all file in directory
       vector<String> filenames;
       glob(in_folder, filenames);
       cout<<filenames.size()<<endl;    //to display no of files


       // read first image to know with and height
       fisheyeImage = imread (filenames[0]);
       Hf = fisheyeImage.size().height;
       Wf = fisheyeImage.size().width;

       // crop first image
       cropped_image.create(Hf, Wf/2, fisheyeImage.type());
       fisheyeImage(Rect(Wf/2 - Hf/2 , 0, Hf, Hf)).copyTo(cropped_image);

       // get With/Height of croped first image
       Wf =     cropped_image.rows;
       Hf =     cropped_image.cols;
       We = Wf;
       He = Hf;

       // creat matrix cordinate
       Mat map_x = Mat::zeros(He, We,  CV_32F);
       Mat map_y = Mat::zeros(He, We,  CV_32F);

       // creat Black image
       int With_Black, Height_Black;
       With_Black   =   We *2;
       Height_Black =   He ;
       Mat3b res(Height_Black, With_Black, Vec3b(0,0,0));
       cout << "res H, W = " << res.rows << "  "<<  res.cols << endl;


       // remap image to theta image
       findCorrespondingFisheyePoint(map_x, map_y, He, We, Hf, Wf, FOV);


       // process all image in the folder
       String save_name;
       Mat save_img;
       for(size_t i=0; i<filenames.size();i++ )
          {
               cout << "filenames[i] = " << filenames[i] << endl;
               // Read image and crop image
               image = imread (filenames[i]);
               image(cv::Rect(image.cols/2 -image.rows/2 ,0,image.rows,image.rows)).copyTo(cropped_image);

               // change image
               remap( cropped_image, convert_img, map_x, map_y, INTER_LINEAR, BORDER_CONSTANT, Scalar(0, 0, 0) );


               // add black image and save image
               convert_img.copyTo(res(Rect(convert_img.cols/2, 0, convert_img.cols ,convert_img.rows)));
               save_name = out_folder + "/out_image" + to_string(i+1)  +".jpg";

               imwrite(save_name,res );
               cout << "Res W , H = " << res.cols<< " " <<  res.rows << endl;
       }

       waitKey(0);
       destroyAllWindows();
       return 0;


#endif


#else

    String folder = "./input/*.jpg";
    vector<String> filenames;
    glob(folder, filenames);
    cout<<filenames.size()<<endl;//to display no of files


//    QDir dir("D:/HUU_UYEN/Software/KIRARI__NINJA_DAITRON"
//             "/FISHEYE_CPP/FISHEYE_SOFTWARE/data/");
//    dir.setNameFilters(QStringList() << "*.png" << "*.jpg");
//    QStringList fileList = dir.entryList();
//    foreach (QString path, fileList)
//    {
//        cout << "fileList " <<  path.data() << endl;
//        cout << "fileList" << fileList.size()<< endl;

//      // do what you want, for example, create a new QLabel here
//    }





    image = imread ("D:/HUU_UYEN/Software/KIRARI__NINJA_DAITRON"
                    "/FISHEYE_CPP/build-FISHEYE_SOFTWARE-Desktop_Qt_5_14_2_MinGW_32_bit-Debug"
                    "/input/image_1.jpg");

    float Pi = 3.14159;
    float degree, ratio_degree, R, Cfx, Cfy;
    int Hf, Wf, new_hei, new_wid;
    degree = 240;
    ratio_degree = degree / 360;
    Hf = image.rows;
    Wf = image.cols;

    R = Hf/2;
    Cfx = Wf/2;
    Cfy = Hf/2;
    new_hei = int(R*ratio_degree);
    new_wid = int(2*Pi*R*ratio_degree);
    Mat map_x = Mat::zeros(new_hei, new_wid,  CV_32F);
    Mat map_y = Mat::zeros(new_hei, new_wid,  CV_32F);

    buildMap_2_img(map_x, map_y, R, Cfx, Cfy, new_hei, new_wid);


    int With_Black, Height_Black;
    With_Black   =   new_wid;
    Height_Black =   new_hei * 2;

    Mat3b res(Height_Black, With_Black, Vec3b(0,0,0));
    cout << "res W, H = " << res.rows << "  "<<  res.cols << endl;
    String name = "./output/image";
    String save_name;
    Mat save_img;

    QElapsedTimer timer;
    timer.start();

    for(size_t i=0; i<filenames.size();i++ )
       {

            image = imread (filenames[i]);
            remap( image, out_Img, map_x, map_y, INTER_LINEAR, BORDER_CONSTANT, Scalar(0, 0, 0) );
            out_Img.copyTo(res(Rect(0, out_Img.rows,   out_Img.cols ,out_Img.rows)));
            save_name = name +  to_string(i+1)  +".jpg";

            resize(res, save_img, Size(5376, 2688), INTER_LINEAR);
            cout << "Res W , H = " << res.cols<< " " <<  res.rows << endl;
            imwrite(save_name,save_img );

    }

    cout << "spend time:   " << timer.elapsed() << "milliseconds" << endl;

#endif
}


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{

    ui->setupUi(this);
    Worker *work = new Worker(5);

    connect(this, SIGNAL(pass_dir( QString, QString, int, int)), work, SLOT(receive_dir(QString ,QString,int, int)));

    connect(work, SIGNAL(incrThreadDone()), this, SLOT(incrThreadDoneChange()));

    connect(this, SIGNAL(stop_signal()), work, SLOT(stopThreads()));

    timer = new QTimer(this);

    // setup signal and slot
    connect(timer, SIGNAL(timeout()),this, SLOT(MyTimerSlot()));
    timer->start(100);

    // msec
    waitKey(0);    // Waits for a keystroke in the window

}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_spinBox_valueChanged(int arg1)
{
    cout << "arg1 " << arg1 << endl;
}


int p = 0;
String *a;

void MainWindow::on_start_clicked()
{
    // reset parameter
    stop_dis  = false;
    done = false;
    ui->progressBar->setValue(0);
    ui->spend_time->setText(QString::number(0));
    timer_1.start();
    int mode_change ;
    QString input_folder, output_folder;

    output_folder  = ui->output_folder->toPlainText();
    output_folder = output_folder.replace("\\", "/");



    cout << "file_count_M " << file_count_M << endl;

    int count = 0;
    for (count = 0; count < file_count_M; count ++)
    {
//        cout << "file name in start : " << load_img_file[count].toStdString() << endl << endl;
        input_folder = load_img_file[count];
        emit pass_dir(input_folder,output_folder,mode_change, count);
    }


    // use default directory
    if ((ui->input_folder->toPlainText().isEmpty()) || (ui->output_folder->toPlainText().isEmpty()) )
    {
        if (ui->input_folder->toPlainText().isEmpty())
                    ui->input_folder->setText("Please, choose input folder" );

        if (ui->output_folder->toPlainText().isEmpty())
                    ui->output_folder->setText("Please, choose output folder" );

    }
    else  // set directory
    {

        // explicitly using the relative name of the current working directory
        input_folder   = ui->input_folder->toPlainText();
        output_folder  = ui->output_folder->toPlainText();


        // handle String
        input_folder = input_folder.replace("\\", "/");
        output_folder = output_folder.replace("\\", "/");


        // check number of file
        vector<String> files;
        glob(input_folder.toStdString(), files);
        ui->number_of_img->setText(QString::number(files.size()));

        cout << "input state " << QDir(input_folder).exists() << endl;
        cout << "outout state " << QDir(output_folder).exists() << endl;

        if ( QDir(input_folder).exists() && QDir(output_folder).exists()  )
        {
            // transfer dir data and mode process to class worker
            mode_change    = ui->spinBox->value();

            Worker *work = new Worker(5);
            connect(work, SIGNAL(incrThreadDone()), this, SLOT(incrThreadDoneChange()));
            connect(work, SIGNAL(run_percent(int)), this, SLOT(get_run_percent(int)));
            work->start();

            print_time = 1;
         }
         {

            if  (QDir(input_folder).exists() == false )
            {
                    cout << " colorr" << endl;
                    ui->input_folder->setTextColor(QColor(255, 0, 0));
                    ui->input_folder->setText("Please, choose input folder!!! ");
            }

            if (QDir(output_folder).exists() == false )
            {
                    ui->output_folder->setTextColor(QColor(255, 0, 0));
                    ui->output_folder->setText("Please, choose output folder!!!");
            }
        }
    }


}



void MainWindow::MyTimerSlot()
{
    if (print_time == 1 && done == false)
    {
        timer_1.elapsed();
        ui->spend_time->setText(QString::number( timer_1.elapsed()/1000));
    }

}

void MainWindow::get_run_percent(int percent)
{
    if (stop_dis  == true)

    {
        QThread::sleep(1);
        ui->progressBar->setValue(100);

    }
    else
        ui->progressBar->setValue(percent);

}

void MainWindow::incrThreadDoneChange()
{
    done = true;
    qDebug() << "incrThreadDone "<< endl;
}

void MainWindow::on_Stop_Button_clicked()
{
//    cout << "on_Stop_Button_clicked" << endl;
    emit stop_signal();
    timer_1.elapsed();
    ui->spend_time->setText(QString::number( timer_1.elapsed()/1000));

    stop_dis  = true;
    print_time = 0;
}

void MainWindow::on_choose_input_clicked()
{

    ui->input_folder->setTextColor(QColor(105, 105, 105));
    QFileDialog dialog(this);
    dialog.setNameFilter(tr("Images (*.png *.xpm *.jpg)"));
    dialog.setViewMode(QFileDialog::Detail);
    dialog.setFileMode(QFileDialog::ExistingFiles);

    if ( QDialog::Accepted == dialog.exec() )
    {
        QStringList filenames = dialog.selectedFiles();
        QStringList::const_iterator it = filenames.begin();
        QStringList::const_iterator eIt = filenames.end();

        int j = 0 ;
        file_count_M = int(filenames.size());
        cout << "total file:  " << file_count_M << endl;

        while ( it != eIt )
        {
            QString fileName = *it++;
            if ( !fileName.isEmpty() )
            {
                   load_img_file[j] = fileName;
                   j ++ ;

            }
        }


        QStringList pieces = load_img_file[0].split( "/" );
        QString neededWord = pieces.value( pieces.length() - 1 );

        QString Dir ;

        for (int i = 0 ; i < pieces.length() - 1; i ++  )
        {
            if (i <pieces.length() - 2 )
                Dir= Dir +  pieces.value( i ) + "/" ;
            else
                 Dir= Dir +  pieces.value( i )  ;
        }
        ui->input_folder->setText(Dir);

//        cout  << "Dir == " << Dir.toStdString() << endl;
//        cout << "neededWord:    " << neededWord.toStdString() << endl;

    }
}

void MainWindow::on_choose_output_clicked()
{
    ui->output_folder->setTextColor(QColor(105, 105, 105));

//    QString filename =  QFileDialog::getOpenFileName(this,"output",QDir::currentPath(),
//             "All files (*.*) ;; images (*.png *.jpg) ;; Document files (*.doc *.rtf)");
//    if( !filename.isNull() )
//      {
//        qDebug() << "selected file path : " << filename.toUtf8();
//      }


//    QString filename_1 =  QFileDialog::getExistingDirectory( this, "output", QDir::currentPath(),
//                                                             QFileDialog::ShowDirsOnly );
//    if( !filename_1.isNull() )
//              {
//                qDebug() << "selected file path : " << filename_1.toUtf8();
//              }
//    ui->output_folder->setText(filename_1);



    QString filename_1 =  QFileDialog::getExistingDirectory( this, "output", QDir::currentPath(),
                                            QFileDialog::DontUseCustomDirectoryIcons );
    if( !filename_1.isNull() )
              {
                qDebug() << "selected file path : " << filename_1.toUtf8();
              }
    ui->output_folder->setText(filename_1);


//    QString filename_1 =  QFileDialog::getExistingDirectory
//            ( this, "output", QDir::currentPath(),
//                                            QFileDialog::DontUseNativeDialog );
//    if( !filename_1.isNull() )
//              {
//                qDebug() << "selected file path : " << filename_1.toUtf8();
//              }
//    ui->output_folder->setText(filename_1);

}
