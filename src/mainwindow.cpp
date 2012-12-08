/* Webcamod, webcam capture plasmoid.
 * Copyright (C) 2011-2012  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://kde-apps.org/content/show.php/Webcamoid?content=144796
 */

#include <KAboutApplicationDialog>
#include <KAboutData>
#include <KConfigSkeleton>
#include <KGlobalSettings>
#include <KIcon>
#include <KLocalizedString>
#include <KNotification>

#include "mainwindow.h"

MainWindow::MainWindow(QObject *parent): QWidget(parent)
{
    this->appEnvironment = new AppEnvironment(this);

    this->setupUi(this);

    if (parent != NULL)
        this->setStyleSheet("QWidget#MainWindow{background-color: "
                            "rgba(0, 0, 0, 0);}");

    this->setWindowTitle(QString("%1 %2").
                         arg(QCoreApplication::applicationName()).
                         arg(QCoreApplication::applicationVersion()));

    this->tools = new V4L2Tools(true, this);

    QObject::connect(this->tools,
                     SIGNAL(devicesModified()),
                     this,
                     SLOT(updateWebcams()));

    QObject::connect(this->tools,
                     SIGNAL(playingStateChanged(bool playing)),
                     this,
                     SLOT(playingStateChanged(bool playing)));

    QObject::connect(this->tools,
                     SIGNAL(recordingStateChanged(bool recording)),
                     this,
                     SLOT(recordingStateChanged(bool recording)));

    QObject::connect(this->tools,
                     SIGNAL(gstError()),
                     this,
                     SLOT(showGstError()));

    QObject::connect(this->tools,
                     SIGNAL(frameReady()),
                     this,
                     SLOT(showFrame()));

    this->setWindowIcon(KIcon("camera-web"));
    this->btnTakePhoto->setIcon(KIcon("camera-photo"));
    this->btnStartStop->setIcon(KIcon("media-playback-start"));
    this->btnVideoRecord->setIcon(KIcon("video-x-generic"));
    this->btnConfigure->setIcon(KIcon("configure"));
    this->btnAbout->setIcon(KIcon("help-about"));

    this->wdgControls->hide();

    foreach (QVariantList webcam, this->tools->captureDevices())
        this->cbxSetWebcam->addItem(webcam.at(1));
}

void MainWindow::addWebcamConfigDialog(KConfigDialog *configDialog)
{
    if (!configDialog)
        return;

    this->cfgWebcamDialog = new WebcamConfig(this->tools, this);

    configDialog->addPage(this->cfgWebcamDialog,
                          this->tr("Webcam Settings"),
                          "camera-web",
                          this->tr("Configure the parameters of the webcam."),
                          false);
}

void MainWindow::addEffectsConfigDialog(KConfigDialog *configDialog)
{
    if (!configDialog)
        return;

    this->cfgEffects = new Effects(this->tools, this);

    QObject::connect(this->tools,
                     SIGNAL(previewFrameReady(const QImage &image,
                                              QString effect)),
                     this->cfgEffects,
                     SLOT(setEffectPreview(const QImage &image,
                                           QString effect)));

    configDialog->addPage(this->cfgEffects,
                          this->tr("Configure Webcam Effects"),
                          "tools-wizard",
                          this->tr("Add funny effects to the webcam"),
                          false);

    QObject::connect(configDialog,
                     SIGNAL(finished()),
                     this,
                     SLOT(stopEffectsPreview()));
}

void MainWindow::addVideoFormatsConfigDialog(KConfigDialog *configDialog)
{
    if (!configDialog)
        return;

    this->cfgVideoFormats = new VideoRecordConfig(this->tools, this);

    configDialog->
           addPage(this->cfgVideoFormats,
                   this->tr("Configure Video Recording Formats"),
                   "video-x-generic",
                   this->tr("Add or remove video formats for recording."),
                   false);
}

void MainWindow::addStreamsConfigDialog(KConfigDialog *configDialog)
{
    this->cfgStreams = new StreamsConfig(this->tools, this);

    configDialog->
           addPage(this->cfgStreams,
                   this->tr("Configure Custom Streams"),
                   "network-workgroup",
                   this->tr("Add or remove local or network live streams."),
                   false);
}

void MainWindow::addGeneralConfigsDialog(KConfigDialog *configDialog)
{
    this->cfgGeneralConfig = new GeneralConfig(this->tools, this);

    configDialog->
           addPage(this->cfgGeneralConfig,
                   this->tr("General Options"),
                   "configure",
                   this->tr("Setup the basic capture options."),
                   false);
}

void MainWindow::addFeaturesInfoDialog(KConfigDialog *configDialog)
{
    this->cfgFeaturesInfo = new FeaturesInfo(this->tools, this);

    configDialog->
           addPage(this->cfgFeaturesInfo,
                   this->tr("Features"),
                   "dialog-information",
                   this->tr("This table will show you what packages you need."),
                   false);
}

void MainWindow::showConfigDialog(KConfigDialog *configDialog)
{
    if (!configDialog)
    {
        KConfigSkeleton *config = KConfigSkeleton("", this);

        KConfigDialog *configDialog = \
                new KConfigDialog(this,
                                  this->tr("%1 Settings").arg(QCoreApplication::
                                                applicationName()).toUtf8().data(),
                                  config);

        configDialog->setWindowTitle(this->tr("%1 Settings").arg(QCoreApplication::
                                            applicationName()).toUtf8().data());
    }

    this->addWebcamConfigDialog(configDialog);
    this->addEffectsConfigDialog(configDialog);
    this->addVideoFormatsConfigDialog(configDialog);
    this->addStreamsConfigDialog(configDialog);
    this->addGeneralConfigsDialog(configDialog);
    this->addFeaturesInfoDialog(configDialog);

    QObject::connect(configDialog,
                     SIGNAL(okClicked()),
                     this,
                     SLOT(saveConfigs()));

    QObject::connect(configDialog,
                     SIGNAL(cancelClicked()),
                     this,
                     SLOT(saveConfigs()));

    configDialog->show();
}

QString MainWindow::saveFile(bool video)
{
    QString curTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh-mm-ss");
    QString defaultSuffix;
    QString filters;
    QString defaultFileName;

    if (video)
    {
        QString videosPath = KGlobalSettings::videosPath();
        videoRecordFormats = this->tools->videoRecordFormats();

        QStringList filtersList;
        bool fst = true;

        foreach (QStringList format, videoRecordFormats)
            foreach (QString s, format.at(0).split(",", QString::SkipEmptyParts))
            {
                s = s.trimmed();
                filtersList << QString("%1 file (*.%2)").arg(s.toUpper()).arg(s.toLower());

                if (fst)
                {
                    defaultSuffix = s.toLower();
                    fst = false;
                }
            }

        filters = filtersList.join(";;");

        defaultFileName = QDir::toNativeSeparators(QString("%1/Video %2.%3").
                                                   arg(videosPath).
                                                   arg(curTime).
                                                   arg(defaultSuffix));
    }
    else
    {
        QString picturesPath = KGlobalSettings::picturesPath();

        filters = "PNG file (*.png);;" \
                  "JPEG file (*.jpg);;" \
                  "BMP file (*.bmp);;" \
                  "GIF file (*.gif)";

        defaultSuffix = "png";

        defaultFileName = QDir::toNativeSeparators(QString("%1/Picture %2.png").
                                                   arg(picturesPath).
                                                   arg(curTime));
    }

    if (defaultSuffix.isEmpty())
        return "";

    QFileDialog saveFileDialog(NULL,
                               this->tr("Save File As..."),
                               defaultFileName,
                               filters);

    saveFileDialog.setModal(true);
    saveFileDialog.setDefaultSuffix(defaultSuffix);
    saveFileDialog.setFileMode(QFileDialog::AnyFile);
    saveFileDialog.setAcceptMode(QFileDialog::AcceptSave);
    saveFileDialog.exec();

    QStringList selectedFiles = saveFileDialog.selectedFiles();

    return (selectedFiles.isEmpty())? "": selectedFiles.at(0);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    QSize size = event->size();
    this->lblFrame->resize(size);

    QRect geometry(0,
                   size.height() - this->wdgControls->height(),
                   size.width(),
                   this->wdgControls->height());

    this->wdgControls->setGeometry(geometry);
}

void MainWindow::enterEvent(QEvent *event)
{
    QWidget::enterEvent(event);

    this->wdgControls->show();
}

void MainWindow::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);

    if (!this->rect().contains(this->mapFromGlobal(QCursor::pos())))
        this->wdgControls->hide();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QWidget::closeEvent(event);

    this->tools->stopVideoRecord();
    event->accept();
}

void MainWindow::showFrame(const QImage &webcamFrame)
{
    if (this->tools->playing())
        this->lblFrame->setPixmap(QPixmap::fromImage(webcamFrame));
}

void MainWindow::updateWebcams()
{
    QString oldDevice = this->tools->curDevName();
    bool timerIsActive = this->tools->playing();
    this->tools->stopCurrentDevice();
    this->cbxSetWebcam->clear();
    QVariantList webcams = this->tools->captureDevices();
    QStringList devices;

    foreach (QVariant webcam, webcams)
    {
        devices << webcam.toStringList().at(0);
        this->cbxSetWebcam->addItem(webcam.toStringList().at(1));
    }

    if (devices.contains(oldDevice) && timerIsActive)
        this->tools->startDevice(oldDevice);
}

void MainWindow::playingStateChanged(bool playing)
{
    if (playing)
    {
        this->btnTakePhoto->setEnabled(true);
        this->btnVideoRecord->setEnabled(true);
        this->btnStartStop->setIcon(KIcon("media-playback-stop"));
    }
    else
    {
        this->btnTakePhoto->setEnabled(false);
        this->btnVideoRecord->setEnabled(false);
        this->btnStartStop->setIcon(KIcon("media-playback-start"));
        this->webcamFrame = QImage();
        this->lblFrame->setPixmap(QPixmap::fromImage(this->webcamFrame));
    }
}

void MainWindow::recordingStateChanged(bool recording)
{
    if (recording)
        this->btnVideoRecord->setIcon(KIcon("media-playback-stop"));
    else
        this->btnVideoRecord->setIcon(KIcon("video-x-generic"));
}

void MainWindow::saveConfigs()
{
    this->tools->saveConfigs();
}

void MainWindow::showGstError()
{
    KNotification::event(KNotification::Error,
                         this->tr("An error has occurred"),
                         this->tr("Please, check the \"Features\" section."),
                         QPixmap(),
                         NULL,
                         KNotification::Persistent);
}

void MainWindow::stopEffectsPreview()
{
    QObject::disconnect(this->tools,
                        SIGNAL(previewFrameReady(const QImage &image,
                                                 QString effect)),
                        this->cfgEffects,
                        SLOT(setEffectPreview(const QImage &image,
                                              QString effect)));
}

void MainWindow::on_btnTakePhoto_clicked()
{
    QImage image(this->webcamFrame);
    QString filename = this->saveFile();

    if (!filename.isEmpty())
        image.save(filename);
}

void MainWindow::on_btnVideoRecord_clicked()
{
    if (this->tools->recording())
        this->tools->stopVideoRecord();
    else
        this->tools->startVideoRecord(this->saveFile(true));
}

void MainWindow::on_cbxSetWebcam_currentIndexChanged(int index)
{
    if (this->tools->playing())
        this->tools->startDevice(this->tools->captureDevices().at(index).toStringList().at(0));
}

void MainWindow::on_btnStartStop_clicked()
{
    if (this->tools->playing())
        this->tools->stopCurrentDevice();
    else
        this->tools->startDevice(this->tools->
                    captureDevices().at(this->cbxSetWebcam->currentIndex()).toStringList().at(0));
}

void MainWindow::on_btnConfigure_clicked()
{
    this->showConfigDialog();
}

void MainWindow::on_btnAbout_clicked()
{
    KAboutData *aboutData = new \
        KAboutData(QCoreApplication::applicationName().toUtf8().data(),
                   QCoreApplication::applicationName().toUtf8().data(),
                   ki18n(QCoreApplication::applicationName()),
                   QCoreApplication::applicationVersion().toUtf8().data(),
                   ki18n(this->tr("webcam capture plasmoid.")),
                   KAboutData::License_GPL_V3,
                   ki18n(this->tr("Copyright (C) 2011-2012  "
                                  "Gonzalo Exequiel Pedone")),
                   ki18n(this->tr("A simple webcam plasmoid and "
                                  "stand alone app for picture and "
                                  "video capture.")),
                   "http://github.com/hipersayanX/Webcamoid",
                   "submit@bugs.kde.org");

    aboutData.setProgramIconName("camera-web");

    KAboutApplicationDialog *aboutDialog = new KAboutApplicationDialog(aboutData, this);
    aboutDialog->show();
}
