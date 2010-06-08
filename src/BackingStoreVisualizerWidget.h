/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef BackingStoreVisualizerWidget_h
#define BackingStoreVisualizerWidget_h

#include <QGraphicsWidget>
#include <QMap>
#include "yberconfig.h"

class QGraphicsWebView;
class BackingStoreTileItem;

class BackingStoreVisualizerWidget : public QGraphicsWidget
{
    Q_OBJECT

public:
    BackingStoreVisualizerWidget(QGraphicsWebView*, QGraphicsItem* parent=0);
    ~BackingStoreVisualizerWidget();

protected Q_SLOTS:
    void tileCreated(unsigned hPos, unsigned vPos);
    void tileRemoved(unsigned hPos, unsigned vPos);
    void tilePainted(unsigned hPos, unsigned vPos);
    void tileCacheViewportScaleChanged();
    void resetCacheTiles();

private:
    Q_DISABLE_COPY(BackingStoreVisualizerWidget)

    void connectSignals();
    void disconnectSignals();
    
    QGraphicsWebView* m_webView;
    QMap<int, BackingStoreTileItem*> m_tileMap;

};

#endif
