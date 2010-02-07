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

#ifndef MainWindow_h_
#define MainWindow_h_

#include <QMainWindow>
#include <QUrl>
#include <QTime>
#include <QTimer>
#include <QLineEdit>
#include <qgraphicswebview.h>
#include <qwebpage.h>

class QGraphicsScene;
class QLineEdit;
class WebPage;
class WebView;
class MainView;
class QNetworkProxy;
class UrlStore;
class QLabel;
class QToolBar;
class QGraphicsView;

class AutoSelectLineEdit;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QNetworkProxy* m_proxy, const QString& url);
    ~MainWindow();

    MainWindow* createWindow();

    QWebPage* page() const;

    MainView* mainView();

    // fixme: this needs to be a singleton.
    UrlStore* urlStore() const { return m_urlStore; }

    static QUrl urlFromUserInput(const QString& string);

public Q_SLOTS:
    void load(const QString& url);
    MainWindow* newWindow(const QString &url = QString());
    void changeLocation();
    void urlTextEdited(const QString&);

    void loadStarted();
    void loadFinished(bool);

    void updateURL();
    void urlChanged(const QUrl& url);
    void showFPSChanged(bool);
    void showTilesChanged(bool);

    void zoomIn() { zoomInOrOut(true); }
    void zoomOut() { zoomInOrOut(false); }
    void zoomInOrOut(bool zoomIn);

    void updateUrlStore();
    
protected:
    void keyPressEvent(QKeyEvent* event);
    void timerEvent(QTimerEvent *event);

#if defined(Q_WS_MAEMO_5)
    bool event(QEvent *ev);
protected Q_SLOTS:
    void orientationChanged(const QString &newOrientation);
#endif

private:
    void init(bool historyOn);
    void setFPSCalculation(bool);
    void buildUI();
    void buildToolbar();
    void setLoadInProgress(bool);

#if defined(Q_WS_MAEMO_5)
    void setLandscape();
    void setPortrait();
    void grabIncreaseDecreaseKeys(QWidget* window, bool grab);
#endif

private:
    MainView* m_mainView;
    QGraphicsScene* m_scene;
    WebView* m_webViewItem;
    WebPage* m_page;
    QNetworkProxy* m_proxy;     // not owned (FIXME)
    QAction* m_stopAction;
    QAction* m_reloadAction;
    QAction* m_historyAction;
    UrlStore* m_urlStore;
    
    QToolBar* m_naviToolbar;
    AutoSelectLineEdit* m_urlEdit;
    QLabel* m_fpsBox;
    QString m_lastEnteredText;

    QTime m_fpsTimestamp;
    unsigned int m_fpsTicks;
    int m_fpsTimerId;

    QTime m_zoomInOutTimestamp;
};

class WebView : public QGraphicsWebView {
    Q_OBJECT

public:
    WebView(QGraphicsItem* parent = 0)
        : QGraphicsWebView(parent)
        , m_fpsTicks(0)
    {
    }
    void paint(QPainter* p, const QStyleOptionGraphicsItem* i, QWidget* w= 0) {
        m_fpsTicks++;
        QGraphicsWebView::paint(p, i, w);
    }

    unsigned int fpsTicks() const { return m_fpsTicks; }

private:
    unsigned int m_fpsTicks;
};

class WebPage : public QWebPage {
    Q_OBJECT

public:
    WebPage(QObject* parent, MainWindow* ownerWindow)
        : QWebPage(parent)
        , m_ownerWindow(ownerWindow)
        {}

    virtual QWebPage* createWindow(QWebPage::WebWindowType);

private:
    MainWindow* m_ownerWindow;
};

class AutoSelectLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    AutoSelectLineEdit(QWidget* parent);

Q_SIGNALS:
    void editCancelled();

protected:
    void focusInEvent(QFocusEvent*e);
    void focusOutEvent(QFocusEvent*e);

private:
    QTimer m_selectURLTimer;
};


#endif
