/*
 * Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2006 George Staikos <staikos@kde.org>
 * Copyright (C) 2006 Dirk Mueller <mueller@kde.org>
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2006 Simon Hausmann <hausmann@kde.org>
 * Copyright (C) 2009 Kenneth Christiansen <kenneth@webkit.org>
 * Copyright (C) 2009 Antonio Gomes <antonio.gomes@openbossa.org>
 * Copyright (C) 2009 Girish Ramakrishnan <girish@forwardbias.in>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsWidget>
#include <QNetworkRequest>
#include <QTextStream>
#include <QVector>
#include <QtGui>
#include <QtNetwork/QNetworkProxy>
#include <cstdio>
#include <qwebelement.h>
#include <qwebframe.h>
#include <qgraphicswebview.h>
#include <qwebpage.h>
#include <qwebsettings.h>
#include <qwebview.h>
#include <qwebhistory.h>

#include <QGLWidget>
#include <QtGlobal>


#include "MainView.h"
#include "WebViewportItem.h"

static const int s_fpsTimerInterval = 1000;

// TODO:
// detect when user interaction has been done and do
// m_state = Interaction;

// to debug load events etc:
// make DEFINES+=-DENABLE_LOADEVENT_DEBUG
// this is needed in order to make load/back/forward etc
// as fast as possible, doing as little zooming changes as possible
// qt api is not very clear on the signal order. once the aspects
// of load signal order have been cleared, remove these ifdefs

MainView::MainView(QWidget* parent, Settings settings)
    : QGraphicsView(parent)
    , m_interactionItem(0)
    , m_state(InitialLoad)
    , m_fpsTicks(0)
    , m_fpsTimerId(0)
    , m_fpsItem(0)
    , m_fpsEnabled(settings.m_showFPS)
    , m_webView(0)
{
    if (settings.m_useGL)
        setViewport(new QGLWidget);

    setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    setOptimizationFlags(QGraphicsView::DontSavePainterState);

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setFrameShape(QFrame::NoFrame);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

MainView::~MainView()
{
    hideFPS();
}

void MainView::setWebView(WebView* webViewItem)
{
    hideFPS();

    if (webViewItem) {
        if (!m_interactionItem) {
            Q_ASSERT(scene());
            m_interactionItem = new WebViewportItem();
            scene()->addItem(m_interactionItem);
            scene()->setActiveWindow(m_interactionItem);
        }
        m_interactionItem->setWebView(m_webView = webViewItem);
        installSignalHandlers();
        updateSize();
        if (m_fpsEnabled)
            showFPS();
    } else {
        hideFPS();
        m_interactionItem->setParent(0);
        delete m_interactionItem;
        m_interactionItem = 0;
    }
}

void MainView::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);
    updateSize();
}

void MainView::updateSize()
{
    setUpdatesEnabled(false);
    if (!m_interactionItem)
        return;

    QRectF rect(QPointF(0, 0), size());
    scene()->setSceneRect(rect);

    m_interactionItem->setGeometry(rect);
    updateZoomScaleToPageWidth();
    setUpdatesEnabled(true);
}

WebView* MainView::webView()
{
    return m_webView;
}

void MainView::installSignalHandlers()
{
    connect(webView()->page()->mainFrame(), SIGNAL(initialLayoutCompleted()), this, SLOT(resetState()));
    connect(webView()->page()->mainFrame(), SIGNAL(contentsSizeChanged(const QSize &)), this, SLOT(contentsSizeChanged(const QSize&)));
    connect(webView()->page(),SIGNAL(saveFrameStateRequested(QWebFrame*,QWebHistoryItem*)), SLOT(saveFrameState(QWebFrame*,QWebHistoryItem*)));
    connect(webView()->page(),SIGNAL(restoreFrameStateRequested(QWebFrame*)), SLOT(restoreFrameState(QWebFrame*)));
    connect(webView(), SIGNAL(loadFinished(bool)), this, SLOT(loadFinished(bool)));
    connect(webView(), SIGNAL(loadStarted()), this, SLOT(loadStarted()));
}

void MainView::resetState()
{
#if defined(ENABLE_LOADEVENT_DEBUG)
    qDebug() << __FUNCTION__;
#endif
    m_state = InitialLoad;
    if (m_interactionItem) {
        // zoom will be updated in contentsSizeChanged
        m_interactionItem->resetState(false);
    }

    setUpdatesEnabled(true);
}

void MainView::loadFinished(bool)
{
#if defined(ENABLE_LOADEVENT_DEBUG)
    qDebug() << __FUNCTION__;
#endif

    if (m_state == InitialLoad)
        m_state = Interaction;
}

void MainView::loadStarted()
{
#if defined(ENABLE_LOADEVENT_DEBUG)
    qDebug() << __FUNCTION__;
#endif
    setUpdatesEnabled(false);
}

void MainView::contentsSizeChanged(const QSize&)
{
#if defined(ENABLE_LOADEVENT_DEBUG)
    qDebug() << __FUNCTION__;
#endif
    // FIXME: QSize& arg vs contentsSize(): these  report different sizes
    // probably scrollbar thing
    if (m_state == InitialLoad) {
        updateZoomScaleToPageWidth();
    }
}

void MainView::updateZoomScaleToPageWidth()
{
    if (!m_interactionItem)
        return;

    QSize contentsSize = webView()->page()->mainFrame()->contentsSize();
    qreal targetScale = 1.;
    if (contentsSize.width()) {
        targetScale = static_cast<qreal>(m_interactionItem->size().width()) / contentsSize.width();
    }
    m_interactionItem->setZoomScale(targetScale);
}


void MainView::showFPS()
{
    Q_ASSERT(!m_fpsItem);

    m_fpsItem = new QGraphicsSimpleTextItem("FPS");
    m_fpsItem->setParentItem(m_interactionItem);
    QFont f = m_fpsItem->font();
    f.setPointSize(14);
    f.setBold(true);
    m_fpsTimerId = startTimer(s_fpsTimerInterval);
    m_fpsTicks = webView()->fpsTicks();
    m_fpsTimestamp.start();
}

void MainView::timerEvent(QTimerEvent *)
{
    double dt = m_fpsTimestamp.restart();
    double dticks = webView()->fpsTicks() - m_fpsTicks;
    double d = 0;
    if (dt)
        d = (dticks *  1000.) / dt;
    m_fpsItem->setText(QString("FPS: %1").arg(d, 0, 'f', 2));

    m_fpsTicks = webView()->fpsTicks();
}

void MainView::hideFPS()
{
    if (m_fpsTimerId)
        killTimer(m_fpsTimerId);
    if (m_fpsItem) {
        m_fpsItem->setParentItem(0);
        delete m_fpsItem;
        m_fpsItem = 0;
    }
}


struct SavedViewState
{
    double zoomScale;
    QPointF panPos;

    SavedViewState(WebViewportItem* item)
        : zoomScale(item->zoomScale())
        , panPos(item->panPos())
    {
    }

    SavedViewState()
        : zoomScale(-1)
    {
    }

    bool isValid() { return zoomScale >= 0; }

    void imposeTo(WebViewportItem* item) const
    {
        item->setZoomScale(zoomScale);
        item->setPanPos(panPos);
    }
};
Q_DECLARE_METATYPE(SavedViewState);  // a bit heavy weight for what this is used for

void MainView::saveFrameState(QWebFrame* frame, QWebHistoryItem* item)
{
#if defined(ENABLE_LOADEVENT_DEBUG)
    qDebug() << __FUNCTION__ << frame << item;
#endif
    if (frame == webView()->page()->mainFrame())
        item->setUserData(QVariant::fromValue(SavedViewState(m_interactionItem)));
#if defined(ENABLE_LOADEVENT_DEBUG)
    else
        qDebug() << "Unknown frame at " << __PRETTY_FUNCTION__;
#endif
}

void MainView::restoreFrameState(QWebFrame* frame)
{
#if defined(ENABLE_LOADEVENT_DEBUG)
    qDebug() << __FUNCTION__ << frame;
#endif

    if (frame == webView()->page()->mainFrame()) {
        QWebHistoryItem item = webView()->page()->history()->currentItem();
        QVariant value = item.userData();

        if (value.canConvert<SavedViewState>()) {
            SavedViewState s = value.value<SavedViewState>();
            if (s.isValid())
                s.imposeTo(m_interactionItem);
        }
    }
#if defined(ENABLE_LOADEVENT_DEBUG)
    else
        qDebug() << "Unknown frame at " << __PRETTY_FUNCTION__;
#endif
}



