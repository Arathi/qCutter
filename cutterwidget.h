#ifndef CUTTERWIDGET_H
#define CUTTERWIDGET_H

#include "formattedtime.h"

#include <QWidget>
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QProcess>
#include <QSettings>
#include <QFile>
#include <QDir>

QT_BEGIN_NAMESPACE
namespace Ui { class CutterWidget; }
QT_END_NAMESPACE

class CutterWidget : public QWidget
{
    Q_OBJECT

public:
    CutterWidget(QWidget *parent = nullptr);
    ~CutterWidget();

private slots:
    void on_pushButtonSelectFFmpeg_clicked();

    void on_pushButtonOpenInputFile_clicked();

    void on_pushButtonOpenOutputFile_clicked();

    void on_pushButtonStartConvert_clicked();

    void on_lineEditScaleWidth_editingFinished();

    void on_lineEditScaleHeight_editingFinished();

    void on_pushButtonPreview_clicked();

private:
    Ui::CutterWidget *ui;
    QSettings *settings;
    QDir *ffmpegDir;

    QString getCommand(
            QString ffmpegPath,
            QString inputFilePath,
            QString startTime,
            QString endTime,
            int scaleWidth,
            int scaleHeight,
            int fps,
            QString outputFilePath
    );

    void loadSettings();
    bool checkFFmpeg();
    QString executeFFmpeg(QString cmd);
    QString getFFmpegVersion(QString ffmpegPath);
    QPair<int, int> getVideoWidthAndHeight(QString ffmpegPath, QString inputFilePath);
    QString getOutputFilePath();
};
#endif // CUTTERWIDGET_H
