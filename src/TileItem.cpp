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

#include "TileItem.h"
#include <QGraphicsWidget>
#include <QPainter>
#include <QtGlobal>
#include <QTimer>
#include <QGraphicsSceneMouseEvent>
#include <QPropertyAnimation>
#include <QGraphicsScene>

#include <QDebug>

const int s_hTextMargin = 10;
const int s_longpressTimeoutThreshold = 400;

TileItem::TileItem(QGraphicsWidget* parent, const UrlItem& urlItem, bool editable)
    : QGraphicsRectItem(parent)
    , m_urlItem(urlItem)
    , m_selected(false)
    , m_closeIcon(0)
    , m_editable(editable)
    , m_context(0)
    , m_dirty(true)
    , m_fixed(false)
{
    setCacheMode(QGraphicsItem::DeviceCoordinateCache);
}

TileItem::~TileItem()
{
    delete m_closeIcon;
}

void TileItem::setEditMode(bool on) 
{ 
    if (!m_editable)
        return;
    if (on && !m_closeIcon) {
        m_closeIcon = new QImage(":/data/icon/48x48/close_item_48.png");
        setEditIconRect();
    } else if (!on) {
        delete m_closeIcon;
        m_closeIcon = 0;
    }
    update();
}

void TileItem::setEditIconRect()
{
    if (!m_closeIcon)
        return;
    m_closeIconRect = QRectF(rect().topRight(), m_closeIcon->rect().size());
    m_closeIconRect.moveRight(m_closeIconRect.right() - m_closeIconRect.width() + 10);
    m_closeIconRect.moveTop(m_closeIconRect.top() - 5);
}

void TileItem::paintExtra(QPainter* painter)
{
    if (m_closeIcon)
        painter->drawImage(m_closeIconRect, *m_closeIcon);

    if (m_selected) {
        painter->setBrush(QColor(80, 80, 80, 160));
        painter->drawRoundRect(rect(), 5, 5);
    }
}

void TileItem::addDropShadow(QPainter& painter, const QRectF rect)
{
    // FIXME: dropshadow shouldnt be a rect
    painter.setPen(QColor(40, 40, 40));
    painter.setBrush(QColor(20, 20, 20));
    QRectF r(rect); r.moveTopLeft(r.topLeft() + QPointF(2,2));
    painter.drawRoundedRect(r, 5, 5);
}

void TileItem::layoutTile()
{
    if (!m_dirty)
        return;
    doLayoutTile();
    setEditIconRect();
    m_dirty = false;
}

void TileItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    // expand it to fit thumbs
    QRectF r(m_closeIconRect);
    r.adjust(-20, -20, 20, 20);

    if (m_longpressTime.elapsed() > s_longpressTimeoutThreshold) {
        emit itemEditingMode(this);
    } else if (m_closeIcon && r.contains(event->pos())) {
        // async item selection, give chance to render the item selected/closed.
        QTimer::singleShot(200, this, SLOT(closeItem()));
    } else {    
        m_selected = true;
        update();
        QTimer::singleShot(200, this, SLOT(activateItem()));
    }
}

void TileItem::mousePressEvent(QGraphicsSceneMouseEvent* /*event*/)
{
    m_longpressTime.start();        
}

void TileItem::activateItem()
{
    emit itemActivated(this);
}

void TileItem::closeItem()
{
    emit itemClosed(this);
}

////////////////////////////////////////////////////////////////////////////////
ThumbnailTileItem::ThumbnailTileItem(QGraphicsWidget* parent, const UrlItem& urlItem, bool editable)
    : TileItem(parent, urlItem, editable)
    , m_defaultIcon(0)
{
    if (!urlItem.thumbnail())
        m_defaultIcon = new QImage(":/data/icon/48x48/defaulticon_48.png");
}

ThumbnailTileItem::~ThumbnailTileItem()
{
    delete m_defaultIcon;
}

void ThumbnailTileItem::doLayoutTile()
{
    QFont f("Nokia Sans", 10);
    QRectF r(rect()); 

    m_textRect = r;
    m_thumbnailRect = r;

    if (m_defaultIcon) {
        m_thumbnailRect.setSize(m_defaultIcon->rect().size());
        m_thumbnailRect.moveCenter(r.center());
    } else {
        // stretch thumbnail
        m_thumbnailRect.adjust(2, 2, -2, -(QFontMetrics(f).height() + 3));
    }
    m_title = QFontMetrics(f).elidedText(m_urlItem.title(), Qt::ElideRight, m_textRect.width());
}

void ThumbnailTileItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/, QWidget* /*widget*/)
{
    layoutTile();

    QRectF r(rect()); 

    // QGraphicsDropShadowEffect doesnt perform well on n900.
    addDropShadow(*painter, r);
 
    painter->setBrush(Qt::white);
    painter->setPen(Qt::black);
    painter->drawRoundedRect(r, 5, 5);
    QRectF thumbnailRect(r);
    painter->drawImage(m_thumbnailRect, *(m_defaultIcon ? m_defaultIcon : m_urlItem.thumbnail()));

    painter->setFont(QFont("Nokia Sans", 10));
    painter->setPen(Qt::black);
    painter->drawText(m_textRect, Qt::AlignHCenter|Qt::AlignBottom, m_title);
    paintExtra(painter);
}

NewWindowTileItem::NewWindowTileItem(QGraphicsWidget* parent, const UrlItem& item)
    : ThumbnailTileItem(parent, item, false)

{
}

void NewWindowTileItem::activateItem()
{
    // start growing anim
    setZValue(100);
    QPropertyAnimation* moveAnim = new QPropertyAnimation(this, "rect");
    moveAnim->setDuration(500);
    moveAnim->setStartValue(rect());
    // FIXME find out how to make it full view, for now just hack it
    QRectF r(parentWidget()->geometry());
    r.adjust(scene()->sceneRect().right() - r.right(), 0, 0, 0);
    moveAnim->setEndValue(r);

    moveAnim->setEasingCurve(QEasingCurve::OutQuad);
    moveAnim->start();
    connect(moveAnim, SIGNAL(finished()), this, SLOT(newWindowAnimFinished()));
}

void NewWindowTileItem::newWindowAnimFinished()
{
    emit itemActivated(this);
}

void NewWindowTileItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/, QWidget* /*widget*/)
{
    layoutTile();

    painter->setPen(Qt::white);
    painter->setBrush(m_selected ? Qt::white : Qt::black);
    painter->drawRoundedRect(rect(), 5, 5);

    painter->setFont(QFont("Nokia Sans", 12));
    painter->setPen(Qt::white);
    painter->drawText(rect(), Qt::AlignCenter, "Open new tab");
}

NewWindowMarkerTileItem::NewWindowMarkerTileItem(QGraphicsWidget* parent, const UrlItem& item)
    : ThumbnailTileItem(parent, item, false)

{
    setFixed(true);
}

void NewWindowMarkerTileItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/, QWidget* /*widget*/)
{
    layoutTile();

    QPen p(Qt::DashLine);
    p.setColor(QColor(100, 100, 100));
    painter->setPen(p);
    painter->setBrush(Qt::NoBrush);
    painter->drawRoundedRect(rect(), 5, 5);
}

//
ListTileItem::ListTileItem(QGraphicsWidget* parent, const UrlItem& urlItem, bool editable)
    : TileItem(parent, urlItem, editable)
{
}

void ListTileItem::doLayoutTile()
{
    QRectF r(rect()); 
    r.adjust(s_hTextMargin, 0, -s_hTextMargin, 0);
    m_titleRect = r;
    m_urlRect = r;

    QFont fbig("Nokia Sans", 18);
    QFont fsmall("Nokia Sans", 12);
    QFontMetrics fmfbig(fbig);
    QFontMetrics fmfsmall(fsmall);

    int fontHeightRatio = r.height() / (fmfbig.height() + fmfsmall.height() + 5);
    m_titleRect.setHeight(fmfbig.height() * fontHeightRatio);
    m_urlRect.setTop(m_titleRect.bottom() + 5); 

    m_title = fmfbig.elidedText(m_urlItem.title(), Qt::ElideRight, r.width());
    m_url = fmfsmall.elidedText(m_urlItem.url().toString(), Qt::ElideRight, r.width());
}

void ListTileItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/, QWidget* /*widget*/)
{
    layoutTile();

    QRectF r(rect()); 

    // QGraphicsDropShadowEffect doesnt perform well on n900.
    addDropShadow(*painter, r);
 
    painter->setBrush(Qt::white);
    painter->setPen(Qt::black);
    painter->drawRoundedRect(r, 3, 3);

    painter->setPen(Qt::black);
    painter->setFont(QFont("Nokia Sans", 18));
    painter->drawText(m_titleRect, Qt::AlignLeft|Qt::AlignVCenter, m_title);

    painter->setPen(QColor(110, 110, 110));
    painter->setFont(QFont("Nokia Sans", 12));
    painter->drawText(m_urlRect, Qt::AlignLeft|Qt::AlignVCenter, m_url);

    paintExtra(painter);
}


