#include "worker.h"
#include "mainwindow.h"
#include <QDebug>
#include <QtConcurrent/QtConcurrent>
#include <QTime>
#include <stdio.h>


#define process 1
#if process

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

#include "worker.h"
using namespace cv;
#endif

using namespace std;
const double PI = 3.1415;
bool stop_convert = false;


#if process

// global variable
int global_He, global_We;

Mat map_x_glo, map_y_glo;
int mode_prcess;

//vector<String> file_names;
int new_hei, new_wid;

QString  output_dir;

String file_names [1000000];
int file_count = 0;


// Algorithm

int vertical_processing(Mat &map_x, Mat &map_y, double He, double We,
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
}


int surround_processing (Mat &map_x, Mat &map_y, int R, float Cfx,
                         float Cfy , int new_hei, int new_wid)
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

// end





#define mode 0
int IMG_coordinates_mode_0 ()
{

      // decale parameter
       int Hf, Wf;       //Height, width and FOV for the input image (=fisheyeImage)
       double FOV = PI;  //FOV in radian
       int He, We;
       Mat fisheyeImage,  cropped_image ;


       // read first image to know with and height
       fisheyeImage = imread (file_names[0]);
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

       global_He = He;
       global_We = We;
       // creat matrix cordinate
       map_x_glo = Mat::zeros(He, We,  CV_32F);
       map_y_glo = Mat::zeros(He, We,  CV_32F);

       if (mode_prcess == 0 )

       // remap image to theta image
       vertical_processing(map_x_glo, map_y_glo, He, We, Hf, Wf, FOV);

       return 0;

}

int IMG_coordinates_mode_1 ()
{

    Mat fisheyeImage;
    float Pi = 3.14159;
    float degree, ratio_degree, R, Cfx, Cfy;
    int Hf, Wf;

    fisheyeImage = imread (file_names[0]);

    degree = 240;
    ratio_degree = degree / 360;
    Hf = fisheyeImage.rows;
    Wf = fisheyeImage.cols;

    R = Hf/2;
    Cfx = Wf/2;
    Cfy = Hf/2;
    new_hei = int(R*ratio_degree);
    new_wid = int(2*Pi*R*ratio_degree);

    map_x_glo = Mat::zeros(new_hei, new_wid,  CV_32F);
    map_y_glo = Mat::zeros(new_hei, new_wid,  CV_32F);

    surround_processing(map_x_glo, map_y_glo, R, Cfx, Cfy, new_hei, new_wid);


    // creat black image
    int With_Black, Height_Black;
    With_Black   =   new_wid;
    Height_Black =   new_hei * 2;
    Mat3b res(Height_Black, With_Black, Vec3b(0,0,0));

    return 0 ;
}
// end
#endif



Worker::Worker(int threads_count)
{
    this->threads = threads_count;
}

QElapsedTimer timer;

void Worker::run()
{
  timer.start();
  qDebug() << "Worker start";


  if (mode_prcess == 0)
    {
        IMG_coordinates_mode_0 ();  // create new coordinates from old image
    }
  else
    {
        cout << "mode process ==  " << mode_prcess << endl;
        IMG_coordinates_mode_1 ();
    }

  cout << "remap time  " << timer.elapsed() << endl;

  pool.addFuture(QtConcurrent::run(this, &Worker::task_1));

  pool.addFuture(QtConcurrent::run(this, &Worker::task_2));

  pool.addFuture(QtConcurrent::run(this, &Worker::task_3));

  pool.addFuture(QtConcurrent::run(this, &Worker::task_4));

  pool.addFuture(QtConcurrent::run(this, &Worker::task_5));

  pool.waitForFinished();

  stop_convert = false;
  cout << "total time  " << timer.elapsed() << endl;
  emit run_percent (100 );
  emit incrThreadDone();
  qDebug() << "Worker end";

}

int task_count =5;
int task_1_status, task_2_status,
    task_3_status, task_4_status;



void Worker::task_1()
{
    cout << "task_1 is runing" << endl;

    if (mode_prcess == 0)
    {
        // creat Black image
        int With_Black, Height_Black;
        With_Black   =   global_We *2;
        Height_Black =   global_He ;
        Mat3b res(Height_Black, With_Black, Vec3b(0,0,0));

        // process all image in the folder
        String save_name;
        Mat  image,cropped_image,convert_img, save_img;

        image = imread (file_names[0]);
        cropped_image.create(global_He, global_We, image.type());
        int per_cent;

        cout << "output:   "  << output_dir.toStdString()<< endl;
        for(int i=0; i<file_count/task_count;i++ )
           {

                // Read image and crop image
                image = imread (file_names[i]);
                image(cv::Rect(image.cols/2 -image.rows/2 ,0,image.rows,image.rows)).copyTo(cropped_image);

                // change image
                remap( cropped_image, convert_img, map_x_glo, map_y_glo, INTER_LINEAR, BORDER_CONSTANT, Scalar(0, 0, 0) );

                // add black image and save image
                convert_img.copyTo(res(Rect(convert_img.cols/2, 0, convert_img.cols ,convert_img.rows)));
                save_name = output_dir.toStdString() + "/out_image" + to_string(i+1)  +".jpg";

                // resize and save image
                resize(res, save_img, Size(5376, 2688), INTER_LINEAR);
                imwrite(save_name,save_img );

                per_cent = int (i*100 /(file_count/task_count));
                emit run_percent (per_cent);

                // stop convert image
                if (stop_convert == true )
                {
                    goto done_task_1;
                }
        }
   }
   else
   {
        cout << "task 1 mode =  " << mode_prcess << endl;
        // creat black image
        int With_Black, Height_Black;
        With_Black   =   new_wid;
        Height_Black =   new_hei * 2;
        Mat3b res(Height_Black, With_Black, Vec3b(0,0,0));

        Mat fisheyeImage;
        Mat save_img, out_Img;
        int per_cent;

        String save_name;
        for(int i=0; i<file_count/task_count;i++ )
           {

            per_cent = int (i*100 /(file_count/task_count));
//                emit run_percent (per_cent);

            // stop convert image
            if (stop_convert == true )
            {
                goto done_task_1;
            }
            else
            {
                emit run_percent (per_cent);

            }

                fisheyeImage = imread (file_names[i]);
                remap( fisheyeImage, out_Img, map_x_glo, map_y_glo, INTER_LINEAR,
                                                                BORDER_CONSTANT, Scalar(0, 0, 0) );

                out_Img.copyTo(res(Rect(0, out_Img.rows,   out_Img.cols ,out_Img.rows)));
                save_name = output_dir.toStdString() + "/out_image" + to_string(i+1)  +".jpg";
                resize(res, save_img, Size(5376, 2688), INTER_LINEAR);

                cout << "save_name =  " << save_name << endl;
                imwrite(save_name,save_img );


           }

   }

done_task_1:
    task_1_status = 1;

}



void Worker::task_2()
{
    cout << "task_2 is runing" << endl;
    if (mode_prcess == 0)
    {
        cout << "task 2 mode =  " << mode_prcess << endl;
        // creat Black image
        int With_Black, Height_Black;
        With_Black   =   global_We *2;
        Height_Black =   global_He ;
        Mat3b res(Height_Black, With_Black, Vec3b(0,0,0));
        cout << "res H, W 222 = " << res.rows << "  "<<  res.cols << endl;

        // process all image in the folder
        String save_name;
        Mat  image,cropped_image,convert_img, save_img;

        image = imread (file_names[0]);
        cropped_image.create(global_He, global_We, image.type());

        for(int i=file_count/task_count; i<file_count*2/task_count;i++ )
           {
                // Read image and crop image
                image = imread (file_names[i]);

                image(cv::Rect(image.cols/2 -image.rows/2 ,0,image.rows,image.rows)).copyTo(cropped_image);

                // change image
                remap( cropped_image, convert_img, map_x_glo, map_y_glo, INTER_LINEAR, BORDER_CONSTANT, Scalar(0, 0, 0) );

                // add black image and save image
                convert_img.copyTo(res(Rect(convert_img.cols/2, 0, convert_img.cols ,convert_img.rows)));
                save_name = output_dir.toStdString() + "/out_image" + to_string(i+1)  +".jpg";


                // resize and save image
                resize(res, save_img, Size(5376, 2688), INTER_LINEAR);
                imwrite(save_name,save_img );

                // stop convert image
                if (stop_convert == true )
                {
                    goto done_task_2;
                }
        }
    }
    else
    {
        cout << "mode = " << mode_prcess << endl;
        cout << "task 1 mode =  " << mode_prcess << endl;
        // creat black image
        int With_Black, Height_Black;
        With_Black   =   new_wid;
        Height_Black =   new_hei * 2;
        Mat3b res(Height_Black, With_Black, Vec3b(0,0,0));

        Mat fisheyeImage;
        Mat save_img, out_Img;

        String save_name;
        for(int i = file_count/task_count; i<file_count*2/task_count;i++ )
           {

                fisheyeImage = imread (file_names[i]);
                remap( fisheyeImage, out_Img, map_x_glo, map_y_glo, INTER_LINEAR,
                                                                BORDER_CONSTANT, Scalar(0, 0, 0) );

                out_Img.copyTo(res(Rect(0, out_Img.rows,   out_Img.cols ,out_Img.rows)));
                save_name = output_dir.toStdString() + "/out_image" + to_string(i+1)  +".jpg";

                // resize and save image
                resize(res, save_img, Size(5376, 2688), INTER_LINEAR);
                imwrite(save_name,save_img );

                // stop convert image
                if (stop_convert == true )
                {
                    goto done_task_2;
                }

           }


    }

done_task_2:
    task_2_status = 1;
}



void Worker::task_3()
{
    cout << "task_3 is runing" << endl;

   if (mode_prcess == 0 )
       {
                cout << "task 3 mode = " << mode_prcess << endl;
               // creat Black image
                int With_Black, Height_Black;
                With_Black   =   global_We *2;
                Height_Black =   global_He ;
                Mat3b res(Height_Black, With_Black, Vec3b(0,0,0));

                // process all image in the folder
                String save_name;
                Mat  image,cropped_image,convert_img, save_img;

                image = imread (file_names[0]);
                cropped_image.create(global_He, global_We, image.type());

                for(int i=file_count*2/task_count;  i<file_count*3/task_count; i++ )
                   {

                        // Read image and crop image
                        image = imread (file_names[i]);
                        image(cv::Rect(image.cols/2 -image.rows/2 ,0,image.rows,image.rows)).copyTo(cropped_image);

                        // change image
                        remap( cropped_image, convert_img, map_x_glo, map_y_glo, INTER_LINEAR, BORDER_CONSTANT, Scalar(0, 0, 0) );


                        // add black image and save image
                        convert_img.copyTo(res(Rect(convert_img.cols/2, 0, convert_img.cols ,convert_img.rows)));
                        save_name = output_dir.toStdString() + "/out_image" + to_string(i+1)  +".jpg";

                        // resize and write
                        resize(res, save_img, Size(5376, 2688), INTER_LINEAR);
                        imwrite(save_name,save_img );

                        // stop convert image
                        if (stop_convert == true )
                        {
                            goto done_task_3;
                        }
                    }
   }
   else
   {
       cout << "mode = " << mode_prcess << endl;

       cout << "task 1 mode =  " << mode_prcess << endl;
       // creat black image
       int With_Black, Height_Black;
       With_Black   =   new_wid;
       Height_Black =   new_hei * 2;
       Mat3b res(Height_Black, With_Black, Vec3b(0,0,0));

       Mat fisheyeImage;
       Mat save_img, out_Img;

       String save_name;
       for(int i = file_count*2/task_count; i<file_count*3/task_count;i++ )
          {

               fisheyeImage = imread (file_names[i]);
               remap( fisheyeImage, out_Img, map_x_glo, map_y_glo, INTER_LINEAR,
                                                               BORDER_CONSTANT, Scalar(0, 0, 0) );

               out_Img.copyTo(res(Rect(0, out_Img.rows,   out_Img.cols ,out_Img.rows)));
               save_name = output_dir.toStdString() + "/out_image" + to_string(i+1)  +".jpg";
               resize(res, save_img, Size(5376, 2688), INTER_LINEAR);

               cout << "save_name =  " << save_name << endl;
               imwrite(save_name,save_img );

               // stop convert image
               if (stop_convert == true )
               {
                   goto done_task_3;
               }
          }


   }

done_task_3:
   task_3_status = 1;

}



void Worker::task_4()
{
    cout << "task_4 is runing" << endl;

    if (mode_prcess == 0)
    {
            cout << "task 4 mode = " << mode_prcess << endl;
            // creat Black image
            int With_Black, Height_Black;
            With_Black   =   global_We *2;
            Height_Black =   global_He ;
            Mat3b res(Height_Black, With_Black, Vec3b(0,0,0));

            // process all image in the folder
            String save_name;
            Mat  image,cropped_image,convert_img, save_img;

            image = imread (file_names[0]);
            cropped_image.create(global_He, global_We, image.type());

            for(int i=file_count*3/task_count; i<file_count*4/task_count;i++ )
               {
                    // Read image and crop image
                    image = imread (file_names[i]);
                    image(cv::Rect(image.cols/2 -image.rows/2 ,0,image.rows,image.rows)).copyTo(cropped_image);

                    // change image
                    remap( cropped_image, convert_img, map_x_glo, map_y_glo, INTER_LINEAR, BORDER_CONSTANT, Scalar(0, 0, 0) );

                    // add black image and save image
                    convert_img.copyTo(res(Rect(convert_img.cols/2, 0, convert_img.cols ,convert_img.rows)));
                    save_name = output_dir.toStdString() + "/out_image" + to_string(i+1)  +".jpg";

                    // resize and write
                    resize(res, save_img, Size(5376, 2688), INTER_LINEAR);
                    imwrite(save_name,save_img );

                    // stop convert image
                    if (stop_convert == true )
                    {
                        goto done_task_4;
                    }
            }
      }
    else
    {

              cout << "task 1 mode =  " << mode_prcess << endl;
              // creat black image
              int With_Black, Height_Black;
              With_Black   =   new_wid;
              Height_Black =   new_hei * 2;
              Mat3b res(Height_Black, With_Black, Vec3b(0,0,0));

              Mat fisheyeImage;
              Mat save_img, out_Img;

              String save_name;
              for(int i = file_count*3/task_count; i<file_count*4/task_count;i++ )
                 {

                      fisheyeImage = imread (file_names[i]);
                      remap( fisheyeImage, out_Img, map_x_glo, map_y_glo, INTER_LINEAR,
                                                                      BORDER_CONSTANT, Scalar(0, 0, 0) );

                      out_Img.copyTo(res(Rect(0, out_Img.rows,   out_Img.cols ,out_Img.rows)));
                      save_name = output_dir.toStdString() + "/out_image" + to_string(i+1)  +".jpg";
                      resize(res, save_img, Size(5376, 2688), INTER_LINEAR);

                      cout << "save_name =  " << save_name << endl;
                      imwrite(save_name,save_img );

                      // stop convert image
                      if (stop_convert == true )
                      {
                          goto done_task_4;
                      }
              }

    }

done_task_4:
    task_4_status = 1 ;
}



void Worker::task_5()
{
    cout << "task_5 is runing" << endl;

    if (mode_prcess == 0)
    {
            cout << "task 4 mode = " << mode_prcess << endl;
            // creat Black image
            int With_Black, Height_Black;
            With_Black   =   global_We *2;
            Height_Black =   global_He ;
            Mat3b res(Height_Black, With_Black, Vec3b(0,0,0));

            // process all image in the folder
            String save_name;
            Mat  image,cropped_image,convert_img, save_img;

            image = imread (file_names[0]);
            cropped_image.create(global_He, global_We, image.type());

            for(int i=file_count*4/task_count; i<file_count;i++ )
               {
                    // Read image and crop image
                    image = imread (file_names[i]);
                    image(cv::Rect(image.cols/2 -image.rows/2 ,0,image.rows,image.rows)).copyTo(cropped_image);

                    // change image
                    remap( cropped_image, convert_img, map_x_glo, map_y_glo, INTER_LINEAR, BORDER_CONSTANT, Scalar(0, 0, 0) );

                    // add black image and save image
                    convert_img.copyTo(res(Rect(convert_img.cols/2, 0, convert_img.cols ,convert_img.rows)));
                    save_name = output_dir.toStdString() + "/out_image" + to_string(i+1)  +".jpg";

                    // resize and write
                    resize(res, save_img, Size(5376, 2688), INTER_LINEAR);
                    imwrite(save_name,save_img );

                    // stop convert image
                    if (stop_convert == true )
                    {
                        goto done_task_5;
                    }
                }
      }
    else
    {

              cout << "task 1 mode =  " << mode_prcess << endl;
              // creat black image
              int With_Black, Height_Black;
              With_Black   =   new_wid;
              Height_Black =   new_hei * 2;
              Mat3b res(Height_Black, With_Black, Vec3b(0,0,0));

              Mat fisheyeImage;
              Mat save_img, out_Img;

              String save_name;
              for(int i = file_count*4/task_count; i<file_count;i++ )
                 {

                      fisheyeImage = imread (file_names[i]);
                      remap( fisheyeImage, out_Img, map_x_glo, map_y_glo, INTER_LINEAR,
                                                                      BORDER_CONSTANT, Scalar(0, 0, 0) );

                      out_Img.copyTo(res(Rect(0, out_Img.rows,   out_Img.cols ,out_Img.rows)));
                      save_name = output_dir.toStdString() + "/out_image" + to_string(i+1)  +".jpg";
                      resize(res, save_img, Size(5376, 2688), INTER_LINEAR);

                      cout << "save_name =  " << save_name << endl;
                      imwrite(save_name,save_img );

                      // stop convert image
                      if (stop_convert == true )
                      {
                          goto done_task_5;
                      }
                 }



    }

done_task_5:
    task_4_status = 1 ;
}




void Worker::receive_dir(QString input, QString output,int mode_p, int count)
{

//     cout << "Signal 3 from " <<  input.toStdString() << endl;
//     cout << "Signal 3 from " <<  output.toStdString() << endl;
//     cout << "mode_p " << mode_p << endl;

//     mode_prcess = mode_p;
//     input_dir   = input;


//     // read all file in directory
//     glob(input_dir.toStdString(), file_names);

    cout << "mode_p, file_count :  " << mode_p << " ; " << count<<  endl;
    cout << "file name W: " << input.toStdString() << endl;
    file_names[count] = input.toStdString();
    file_count = count + 1;
    output_dir  = output;

}



void Worker::pauseThreads()
{
    pause = 1;
}

void Worker::resumeThreads()
{
  pause = 0;
  cond.wakeAll();
}

void Worker::stopThreads()
{

  cout << "stopThreads " << endl;
  stop_convert = true;

//  sync.lock();

//  if (!stopped.load())
//  {
//    stopped = 1;

//    qDebug() << "Stop threads: start";

//    if (pause)
//      resumeThreads();

//    for (auto f : pool.futures())
//      f.cancel();

//    qDebug() << "Stop threads: end";
//  }

//  sync.unlock();
}
