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

#ifndef TileContainerWidget_h_
#define TileContainerWidget_h_

#include <QGraphicsWidget>
#include "TileItem.h"

class QGraphicsSceneMouseEvent;
class QParallelAnimationGroup;

class TileBaseWidget : public QGraphicsWidget {
    Q_OBJECT
public:
    virtual ~TileBaseWidget();

    virtual void addTile(TileItem& newItem);
    virtual void removeTile(const TileItem& removed);
    virtual void removeAll();
    virtual bool contains(TileItem& item);
    virtual void layoutTiles() = 0;

    void setEditMode(bool on);
    bool editMode() const { return m_editMode; }

    // FIXME really really ugly way of checking whether 
    // tiles got moved. can't trust mouse events on symbian
    // hopefully, a better solution comes along soon
    void setGeometry(const QRectF& rect) { QGraphicsWidget::setGeometry(rect); m_moved = true; }
    void checkMovedStart() { m_moved = false; }
    bool moved() { return m_moved; }
    //
Q_SIGNALS:
    void closeWidget();

protected:
    TileBaseWidget(const QString& title, QGraphicsItem* parent, Qt::WindowFlags wFlags = 0);

    int titleVMargin();
    int tileTopVMargin();
    QSize doLayoutTiles(const QRectF& rect, int hTileNum, int vTileNum, int marginX, int marginY, int fixedItemWidth = -1, int fixedItemHeight = -1);

private:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);
    void mousePressEvent(QGraphicsSceneMouseEvent*);
    void addMoveAnimation(TileItem& item, int delay, const QPointF& oldPos, const QPointF& newPos);

private Q_SLOTS:
    void adjustContainerHeight();

protected:
    TileList m_tileList;
    QString m_title;
    QRectF m_titleRect;

private:
    QParallelAnimationGroup* m_slideAnimationGroup;
    bool m_editMode;
    bool m_moved;
};

// subclasses
// #########
class TabWidget : public TileBaseWidget {
    Q_OBJECT
public:
    TabWidget(QGraphicsItem* parent = 0, Qt::WindowFlags wFlags = 0);
    void layoutTiles();

    void removeTile(const TileItem& removed);
};

class HistoryWidget : public TileBaseWidget {
    Q_OBJECT
public:
    HistoryWidget(QGraphicsItem* parent = 0, Qt::WindowFlags wFlags = 0);
    
    void removeTile(const TileItem& removed);
    void layoutTiles();
};

class BookmarkWidget : public TileBaseWidget {
    Q_OBJECT
public:
    BookmarkWidget(QGraphicsItem* parent = 0, Qt::WindowFlags wFlags = 0);

    void removeTile(const TileItem& removed);
    void layoutTiles();
};

class PopupWidget : public TileBaseWidget {
    Q_OBJECT
public:
    PopupWidget(QGraphicsItem* parent = 0, Qt::WindowFlags wFlags = 0);

    void removeTile(const TileItem& removed);
    void layoutTiles();
};

#endif

