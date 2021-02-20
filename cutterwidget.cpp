#include "cutterwidget.h"
#include "ui_cutterwidget.h"

CutterWidget::CutterWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CutterWidget)
{
    ui->setupUi(this);

    // 禁用最大化按钮
    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);

    // 禁止调节窗口大小
    setFixedSize(this->width(), this->height());

    ui->lineEditStartTime->setInputMask("99:99:99.000");
    ui->lineEditEndTime->setInputMask("99:99:99.000");

    // TODO 读取配置文件获取FFmpeg
    loadSettings();

    // 设置
    QString ffmpegPath = settings->value("ffmpeg-path", "").toString();
    ui->lineEditFFmpegPath->setText(ffmpegPath);
}

CutterWidget::~CutterWidget()
{
    delete ui;
}

void CutterWidget::on_pushButtonSelectFFmpeg_clicked()
{
    QString filter = "可执行文件(ffmpeg.exe);;所有文件(*.*)";
    QString ffmpegPath = QFileDialog::getOpenFileName(
                this,
                "请选择ffmpeg可执行文件",
                "./ffmpeg/bin",
                filter
    );

    QString version = getFFmpegVersion(ffmpegPath);
    if (!version.isEmpty())
    {
        QMessageBox::information(this, "提示", "FFmpeg版本为" + version);
        settings->setValue("ffmpeg-path", ffmpegPath);
        ui->lineEditFFmpegPath->setText(ffmpegPath);
        ffmpegDir = new QDir(ffmpegPath);
        ffmpegDir->cdUp();
    }
    else
    {
        QMessageBox::warning(this, "警告", "FFmpeg无效！");
    }
}

void CutterWidget::on_pushButtonOpenInputFile_clicked()
{
    QString dir = settings->value("last-input-dir", "").toString();

    QString filter = "MP4文件(*.mp4);;Matroska文件(*.mkv);;所有文件(*.*)";
    QString inputFilePath = QFileDialog::getOpenFileName(
                this,
                "请选择输入视频文件",
                dir,
                filter
    );
    ui->lineEditInputFilePath->setText(inputFilePath);

    // TODO 调用ffprobe获取视频信息

    // 保存输入目录
    QDir lastInputFileDir(inputFilePath);
    lastInputFileDir.cdUp();
    settings->setValue("last-input-dir", lastInputFileDir.path());
}

void CutterWidget::on_pushButtonOpenOutputFile_clicked()
{
    ui->lineEditOutputFilePath->setText(getOutputFilePath());
}

QString CutterWidget::getOutputFilePath() {
    QString dir = settings->value("last-output-dir", "").toString();

    QString filter = "gif文件(*.gif);;webm文件(*.webm);;apng文件(*.apng);;MP4文件(*.mp4);;所有文件(*.*)";
    QString outputFilePath = QFileDialog::getSaveFileName(
                this,
                "请选择输出文件",
                dir,
                filter
    );

    // 保存输出目录
    if (!outputFilePath.isEmpty())
    {
        QDir lastOutputFileDir(outputFilePath);
        lastOutputFileDir.cdUp();
        settings->setValue("last-output-dir", lastOutputFileDir.path());
    }

    return outputFilePath;
}

void CutterWidget::on_pushButtonStartConvert_clicked()
{
    // QString ffmpegPath = ui->lineEditFFmpegPath->text().trimmed();
    // if (ffmpegPath.isEmpty())
    // {
    //     QMessageBox::warning(this, "警告", "未选择FFmpeg");
    //     return;
    // }
    //
    // QString ffmpegVersion = getFFmpegVersion(ffmpegPath);
    // if (ffmpegVersion.isEmpty())
    // {
    //     QMessageBox::warning(this, "警告", "无效的FFmpeg");
    //     return;
    // }
    //
    // ffmpegDir = new QDir(ffmpegPath);
    // ffmpegDir->cdUp();

    if (!checkFFmpeg())
    {
        return;
    }

    QString inputFilePath = ui->lineEditInputFilePath->text().trimmed();
    if (inputFilePath.isEmpty())
    {
        QMessageBox::warning(this, "警告", "请选择输入视频文件");
        return;
    }

    // QPair<int, int> resolution = getVideoWidthAndHeight(ffmpegPath, inputFilePath);

    QString startTimeStr = ui->lineEditStartTime->text().trimmed();
    QString endTimeStr = ui->lineEditEndTime->text().trimmed();
    FormattedTime startTime(startTimeStr);
    FormattedTime endTime(endTimeStr);
    int startTimeMs = startTime.toMillisecond();
    int endTimeMs = endTime.toMillisecond();
    int duration = endTimeMs - startTimeMs;
    qDebug() << "选取片段时长：" << QString::number(duration) << "ms";

    QString scaleWidthStr = ui->lineEditScaleWidth->text().trimmed();
    QString scaleHeightStr = ui->lineEditScaleHeight->text().trimmed();
    QString fpsStr = ui->lineEditFps->text().trimmed();

    int scaleWidth = scaleWidthStr.toInt();
    int scaleHeight = scaleHeightStr.toInt();
    int fps = fpsStr.toInt();

    QString outputFilePath = ui->lineEditOutputFilePath->text().trimmed();
    if (outputFilePath.isEmpty())
    {
        QMessageBox::warning(this, "警告", "请选择输出文件");
        return;
    }

    ui->pushButtonStartConvert->setEnabled(false);

    QString ffmpegPath = ffmpegDir->path() + "/ffmpeg" + ".exe";
    QString cmd = getCommand(
                ffmpegPath,
                inputFilePath,
                startTimeStr,
                endTimeStr,
                scaleWidth,
                scaleHeight,
                fps,
                outputFilePath
    );

    qDebug() << "生成命令如下：";
    qDebug().noquote() << cmd;

    int count = 0;
    QProcess process(this);
    connect(&process, &QProcess::readyReadStandardError, this, [&process, this, duration, &count]() {
        QString output = QString::fromLocal8Bit(process.readAllStandardError());
        qDebug().noquote() << "第" << QString::number(++count) << "次输出：" << output;

        QRegExp regex("time=([0-9:\\.]+)");
        if (regex.indexIn(output) != -1)
        {
            if (output.indexOf("Lsize") != -1)
            {
                // 输出完成
                this->ui->progressBar->setMinimum(0);
                this->ui->progressBar->setMaximum(100);
                this->ui->progressBar->setValue(100);
            }
            else
            {
                // 输出进度
                QString current = regex.cap(1);
                FormattedTime t(current);
                int progress = t.toMillisecond() * 100 / duration;
                this->ui->progressBar->setMinimum(0);
                this->ui->progressBar->setMaximum(100);
                this->ui->progressBar->setValue(progress);
            }
        }
        else
        {
            this->ui->progressBar->setMinimum(0);
            this->ui->progressBar->setMaximum(0);
        }
    });

    process.start(cmd);
    process.waitForStarted();
    qDebug() << "ffmpeg进程已启动";

    process.waitForFinished();
    qDebug() << "ffmpeg命令执行完成";

    ui->pushButtonStartConvert->setEnabled(true);
}

void CutterWidget::loadSettings()
{
    settings = new QSettings("config.ini", QSettings::IniFormat);
}

QString CutterWidget::getCommand(
        QString ffmpegPath,
        QString inputFilePath,
        QString startTime,
        QString endTime,
        int scaleWidth,
        int scaleHeight,
        int fps,
        QString outputFilePath
)
{
    QString cmd("");

    // FFmpeg路径
    cmd.append("\"" + ffmpegPath + "\" ");

    // 开始时间
    cmd.append("-ss " + startTime + " ");

    // 结束时间
    cmd.append("-to " + endTime + " ");

    // 输入文件
    cmd.append("-i \"" + inputFilePath + "\" ");

    // 控制缩放
    if (scaleWidth != 0 && scaleHeight != 0)
    {
        cmd.append("-vf scale=" + QString::number(scaleWidth) + ":" + QString::number(scaleHeight) + " ");
    }

    // 控制帧率
    if (fps > 0)
    {
        cmd.append("-r " + QString::number(fps) + " ");
    }

    // 输出视频
    if (outputFilePath.endsWith(".mp4"))
    {
        cmd.append("-vcodec copy -acodec copy ");
    }

    // 覆盖文件
    cmd.append("-y ");

    // 输出文件
    cmd.append("\"" + outputFilePath + "\"");

    return cmd;
}

QString CutterWidget::getFFmpegVersion(QString ffmpegPath)
{
    QString version("");

    QString err = executeFFmpeg("\"" + ffmpegPath + "\"");
    QRegExp regex("ffmpeg version ([0-9A-Za-z\\-\\.]+) Copyright");

    if (regex.indexIn(err) != -1)
    {
        version = regex.cap(1);
        qDebug() << "获取到FFmpeg版本为：" << version;
    }

    return version;
}

QPair<int, int> CutterWidget::getVideoWidthAndHeight(QString ffmpegPath, QString inputFilePath)
{
    int width = 0;
    int height = 0;

    QString cmd("");
    cmd.append("\"" + ffmpegPath + "\" ");
    cmd.append("-i \"" + inputFilePath + "\"");
    QString err = executeFFmpeg(cmd);

    QPair<int, int> pair = QPair<int, int>(width, height);
    return pair;
}

QString CutterWidget::executeFFmpeg(QString cmd)
{
    QProcess process(this);
    qDebug() << "执行命令：";
    qDebug().noquote() << cmd;
    process.start(cmd);
    process.waitForStarted();
    process.waitForFinished();
    QString error = QString::fromLocal8Bit(process.readAllStandardError());
    qDebug() << "ffmpeg输出如下：";
    qDebug().noquote() << error;
    return error;
}

void CutterWidget::on_lineEditScaleWidth_editingFinished()
{
    int width = ui->lineEditScaleWidth->text().trimmed().toInt();
    if (width % 2 == 1) {
        width += 1;
        ui->lineEditScaleWidth->setText(QString::number(width));
    }
    if (width > 0 && ui->checkBox->isChecked()) {
        ui->lineEditScaleHeight->setText("-1");
    }
}

void CutterWidget::on_lineEditScaleHeight_editingFinished()
{
    int height = ui->lineEditScaleHeight->text().trimmed().toInt();
    if (height % 2 == 1) {
        height += 1;
        ui->lineEditScaleHeight->setText(QString::number(height));
    }
    if (height > 0 && ui->checkBox->isChecked()) {
        ui->lineEditScaleWidth->setText("-1");
    }
}

bool CutterWidget::checkFFmpeg() {
    QString ffmpegPath = ui->lineEditFFmpegPath->text().trimmed();
    if (ffmpegPath.isEmpty())
    {
        QMessageBox::warning(this, "警告", "未选择FFmpeg");
        return false;
    }

    QString ffmpegVersion = getFFmpegVersion(ffmpegPath);
    if (ffmpegVersion.isEmpty())
    {
        QMessageBox::warning(this, "警告", "无效的FFmpeg");
        return false;
    }

    ffmpegDir = new QDir(ffmpegPath);
    ffmpegDir->cdUp();

    return true;
}

void CutterWidget::on_pushButtonPreview_clicked()
{
    if (!checkFFmpeg())
    {
        return;
    }

    QString inputFilePath = ui->lineEditInputFilePath->text().trimmed();
    if (inputFilePath.isEmpty())
    {
        QMessageBox::warning(this, "警告", "请选择输入视频文件");
        return;
    }

    QString startTimeStr = ui->lineEditStartTime->text().trimmed();
    QString endTimeStr = ui->lineEditEndTime->text().trimmed();
    FormattedTime startTime(startTimeStr);
    FormattedTime endTime(endTimeStr);
    int startTimeMs = startTime.toMillisecond();
    int endTimeMs = endTime.toMillisecond();
    int duration = endTimeMs - startTimeMs;
    qDebug() << "选取片段时长：" << QString::number(duration) << "ms";

    QString scaleWidthStr = ui->lineEditScaleWidth->text().trimmed();
    QString scaleHeightStr = ui->lineEditScaleHeight->text().trimmed();
    QString fpsStr = ui->lineEditFps->text().trimmed();

    int scaleWidth = scaleWidthStr.toInt();
    int scaleHeight = scaleHeightStr.toInt();
    int fps = fpsStr.toInt();

    QString ffPlayPath = ffmpegDir->path() + "/ffplay" + ".exe";

    QString cmd("");

    // FFplay可执行文件
    cmd.append("\"" + ffPlayPath + "\" ");

    // 开始时间
    cmd.append("-ss " + startTimeStr + " ");

    // 播放时长
    cmd.append("-t " + QString::number(duration / 1000.0) + " ");

    // 输入文件
    cmd.append("-i \"" + inputFilePath + "\"");

    qDebug() << "执行命令如下：";
    qDebug().noquote() << cmd;

    QProcess process(this);
    process.execute(cmd);
}
