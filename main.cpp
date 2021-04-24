#include <QApplication>
#include <QDebug>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/opencv.hpp>

#include "mainwindow.h"
/*--------------------------ffmepg-------------------------------*/

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/time.h"
#include "libswscale/swscale.h"
}
#if 1
int XError(int errNum) {
    char buf[1024] = {0};
    av_strerror(errNum, buf, sizeof(buf));
    qDebug() << buf;
    return 1;
}
double r2Double(AVRational val) {
    if (val.den == 0)
        return 0;
    else
        return val.num / (double)val.den;
}
int test() {
    const char* inurl = "E:/ffmpeg/ffmpeg-4.3.1/bin/1.mp4";
    const char* outurl = "rtmp://192.168.50.189/live/test1";

    cv::VideoCapture cam;
    cv::Mat frame;
    cv::namedWindow("显示", CV_WINDOW_AUTOSIZE);

    //像素格式转换上下文
    SwsContext* swsCtx = NULL;
    AVCodec* codec = NULL;
    AVCodecContext* codecCtx = NULL;
    AVFormatContext* oFmtCtx = NULL;
    avcodec_register_all();
    avformat_network_init();
    try {
        /*--打开视频源--*/
        cam.open(0);
        if (!cam.isOpened())
            throw std::exception("camp open failed");
        std::cout << "cam open successed" << std::endl;

        int inWidth = cam.get(CV_CAP_PROP_FRAME_WIDTH);
        int inHeight = cam.get(CV_CAP_PROP_FRAME_HEIGHT);
        int fps = cam.get(CV_CAP_PROP_FPS);

        /*--格式转换上下文初始化,直接给转成yuv的--*/
        swsCtx = sws_getCachedContext(swsCtx, inWidth, inHeight, AV_PIX_FMT_BGR24, inWidth, inHeight,
                                      AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
        if (!swsCtx) {
            throw std::exception("sws get cache context failed\n");
        }

        //为输出帧 初始化格式等
        AVFrame* yuvFrame = NULL;
        yuvFrame = av_frame_alloc();    //分配内存空间
        yuvFrame->format = AV_PIX_FMT_YUV420P;
        yuvFrame->width = inWidth;
        yuvFrame->height = inHeight;
        yuvFrame->pts = 0;

        int ret = av_frame_get_buffer(yuvFrame, 32);    //分布图像内存空间
        if (ret != 0) {
            char errbuf[1024] = {0};
            av_strerror(ret, errbuf, sizeof(errbuf));
            throw std::exception(errbuf);
        }

        /*--编码器--*/
        codec = avcodec_find_encoder(AV_CODEC_ID_H264);
        if (!codec) {
            throw std::exception("cant find h264 encoder!");
        }
        codecCtx = avcodec_alloc_context3(codec);
        if (!codecCtx) {
            throw std::exception("cant find h264 encoder context!");
        }

        //配置编码器的参数
        codecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;    //全局头
        codecCtx->codec_id = codec->id;
        codecCtx->thread_count = 8;    //开八个编码线程

        codecCtx->bit_rate = 50 * 1024 * 8;    //压缩后的bit位,压缩率
        codecCtx->width = inWidth;             //
        codecCtx->height = inHeight;           //
        codecCtx->time_base = {1, fps};
        codecCtx->framerate = {fps, 1};

        codecCtx->gop_size = 50;       //多少帧一个关键帧
        codecCtx->max_b_frames = 0;    //把b帧设置为0，不然容易出问题
        codecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

        /*--打开编码器上下文--*/
        ret = avcodec_open2(codecCtx, 0, 0);
        if (ret < 0) {
            XError(ret);
            throw std::exception("err");
        }

        /*--配置输出视频流-*/
        ret = avformat_alloc_output_context2(&oFmtCtx, NULL, "flv", outurl);
        if (ret < 0) {
            XError(ret);
            throw std::exception("err");
        }

        AVStream* vStream = avformat_new_stream(oFmtCtx, NULL);    //创建一个流作为视频流
        if (!vStream) {
            XError(0);
            throw std::exception("err");
        }
        vStream->codecpar->codec_tag = 0;
        avcodec_parameters_from_context(vStream->codecpar, codecCtx);    //给流拷贝已经在编码器上下文里面已经搞定的参数
        av_dump_format(oFmtCtx, 0, outurl, 1);                           //打印输出流信息

        //以写入方式，打开url
        ret = avio_open(&oFmtCtx->pb, outurl, AVIO_FLAG_WRITE);
        if (ret < 0) {
            XError(ret);
            throw std::exception("err");
        }
        ret = avformat_write_header(oFmtCtx, NULL);    //给封装添加头
        if (ret < 0) {
            XError(ret);
            throw std::exception("err");
        }

        AVPacket pkt;
        memset(&pkt, 0, sizeof(pkt));
        int vPts = 0;    //视频流的pts

        cv::Mat frame;
        /*--输入图像显示--*/
        for (;;) {
            if (!cam.grab())    //输入视频抓取
                continue;
            if (!cam.retrieve(frame))
                continue;

            cv::imshow("显示", frame);
            cv::waitKey(1);

            unsigned char* inData[AV_NUM_DATA_POINTERS] = {0};    //一个指针数组,每个元素都是一个指向数据的指针
            inData[0] = frame.data;
            int inSize[AV_NUM_DATA_POINTERS] = {0};    //一个指针数组,每个元素都是一个指向数据的指针
            inSize[0] = frame.cols * frame.elemSize();

            //开始对图像进行转换
            ret = sws_scale(swsCtx, inData, inSize, 0, frame.rows, yuvFrame->data, yuvFrame->linesize);
            if (ret < 0) {
                std::cout << "sws scale failed\n";
                XError(ret);
                continue;
            }
            //转换完成后，对yuvFrame的pts赋值
            yuvFrame->pts = vPts;
            vPts++;

            //开始对原始图像进行编码
            ret = avcodec_send_frame(codecCtx, yuvFrame);    //发送到编码池
            if (ret < 0) {
                std::cout << "send frame failed\n";
                XError(ret);
                continue;
            }

            ret = avcodec_receive_packet(codecCtx, &pkt);    //拿出压好的包
            if (ret < 0) {
                std::cout << "receive pkt failed\n";
                XError(ret);
                continue;
            }
            //对pkt进行同步,以编码器时间基为基准
            pkt.pts = av_rescale_q(pkt.pts, codecCtx->time_base, vStream->time_base);
            pkt.dts = av_rescale_q(pkt.dts, codecCtx->time_base, vStream->time_base);
            pkt.duration = av_rescale_q(pkt.duration, codecCtx->time_base, vStream->time_base);

            ret = av_interleaved_write_frame(oFmtCtx, &pkt);    //把帧写入
            if (ret < 0) {
                XError(ret);
                continue;
            } else if (ret == 0) {
                std::cout << "#" << std::flush;
            }
        }

    } catch (std::exception& ex) {
        if (cam.isOpened())
            cam.release();
        if (swsCtx)
            sws_freeContext(swsCtx);
        if (codecCtx) {
            avio_closep(&(oFmtCtx->pb));
            avcodec_free_context(&codecCtx);
        }
        std::cerr << ex.what() << std::endl;
    }
}

#endif
/*---------------------------------------------------------------*/
int main(int argc, char* argv[]) {
    QApplication a(argc, argv);
    // MainWindow w;
    // w.show();
    test();

    return a.exec();
}
