/*
 * Copyright (C) 2018-2019 Michał Siejak
 * This file is part of Quartz - a raytracing aspect for Qt3D.
 * See LICENSE file for licensing information.
 */

#include "renderwindow.h"
#include <Qt3DRaytrace/qraytraceaspect.h>

#include <QScopedPointer>

#include <QApplication>
#include <QMessageBox>
#include <QTimer>

#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QDir>

namespace Config {
static constexpr int UpdateTitleInterval = 200;
} // Config

RenderWindow::RenderWindow()
{
    QTimer *updateTitleTimer = new QTimer(this);
    QObject::connect(updateTitleTimer, &QTimer::timeout, this, &RenderWindow::updateTitle);
    updateTitleTimer->start(Config::UpdateTitleInterval);
}

QVulkanInstance *RenderWindow::createDefaultVulkanInstance()
{
    QScopedPointer<QVulkanInstance> vkInstance(new QVulkanInstance);
    vkInstance->setApiVersion(QVersionNumber(1, 1));
#ifdef QUARTZ_DEBUG
    vkInstance->setLayers(QByteArrayList() << "VK_LAYER_LUNARG_standard_validation");
#endif
    if(!vkInstance->create()) {
        QMessageBox::critical(nullptr, tr("Quartz"), QString("%1: %2")
                              .arg(tr("Failed to create Vulkan instance"))
                              .arg(vkInstance->errorCode()));
        return nullptr;
    }
    return vkInstance.take();
}

bool RenderWindow::chooseSourceFile()
{
    QFileDialog dialog(nullptr, tr("Open QML file"));
    dialog.setNameFilter("QML Files (*.qml)");
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setDirectory(QDir::currentPath());
    if(dialog.exec()) {
        const QString path = dialog.selectedFiles().at(0);
        return setSourceFile(path);
    }
    return false;
}

bool RenderWindow::setSourceFile(const QString &path)
{
    const QString absolutePath = QDir::current().absoluteFilePath(path);
    {
        QFile sceneFile(absolutePath);
        if(!sceneFile.open(QFile::ReadOnly)) {
            QMessageBox::critical(nullptr, tr("Quartz"), QString("%1: %2")
                                  .arg(tr("Cannot open input file"))
                                  .arg(absolutePath));
            return false;
        }
    }

    QDir::setCurrent(QFileInfo(absolutePath).absolutePath());

    setSource(QUrl::fromLocalFile(absolutePath));
    setSceneName(QFileInfo(absolutePath).fileName());
    return true;
}

void RenderWindow::setSceneName(const QString &name)
{
    m_sceneName = name;
    updateTitle();
}

void RenderWindow::updateTitle()
{
    Qt3DRaytrace::QRenderStatistics statistics;
    if(raytraceAspect()->queryRenderStatistics(statistics)) {
        double frameTime = std::max(statistics.cpuFrameTime, statistics.gpuFrameTime);
        if(qFuzzyIsNull(frameTime)) {
            return;
        }

        QString statisticsString = QString("CPU: %1 ms | GPU: %2 ms | FPS: %3 | Current image: %5 s / %6 spp")
                .arg(statistics.cpuFrameTime, 0, 'f', 2)
                .arg(statistics.gpuFrameTime, 0, 'f', 2)
                .arg(1000.0 / frameTime, 0, 'f', 0)
                .arg(statistics.totalRenderTime, 0, 'f', 2)
                .arg(statistics.numFramesRendered);

        setTitle(QString("%1 - %2 [ %3 ]")
                 .arg(m_sceneName)
                 .arg(QApplication::applicationName())
                 .arg(statisticsString));
    }
    else {
        setTitle(QString("%1 - %2")
                 .arg(m_sceneName)
                 .arg(QApplication::applicationName()));
    }
}
