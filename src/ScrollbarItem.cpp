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

#include "ScrollbarItem.h"

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsWidget>
#include <QPropertyAnimation>
#include <QDebug>

static const qreal s_thumbWidth = 7;
static const qreal s_thumbMinSize = 20;
static const qreal s_thumbDefaultMargin = 12;

const unsigned s_scrollbarFadeTimeout = 300; // in msec
const unsigned s_scrollbarFadeDuration = 300; // in msec

const qreal s_scrollbarOpacityStart = 0.2;
const qreal s_scrollbarOpacity = 0.8;

ScrollbarItem::ScrollbarItem(Qt::Orientation orientation, QGraphicsItem* parent)
    : QGraphicsRectItem(parent)
    , m_orientation(orientation)
    , m_fadeAnim(this, "opacity")
    , m_fadeOutTimeout(this)
    , m_topMargin(0)
    , m_bottomMargin(s_thumbDefaultMargin)
{
#ifndef Q_OS_SYMBIAN
    setCacheMode(QGraphicsItem::DeviceCoordinateCache);
#endif
    m_fadeAnim.setDuration(s_scrollbarFadeDuration);
    m_fadeAnim.setStartValue(s_scrollbarOpacityStart);
    m_fadeAnim.setEndValue(s_scrollbarOpacity);
    m_fadeAnim.setEasingCurve(QEasingCurve::Linear);
    connect(&m_fadeAnim, SIGNAL(finished()), this, SLOT(fadingFinished()));

    m_fadeOutTimeout.setInterval(s_scrollbarFadeTimeout);
    m_fadeOutTimeout.setSingleShot(true);
    connect(&m_fadeOutTimeout, SIGNAL(timeout()), this, SLOT(startFadeOut()));
}

ScrollbarItem::~ScrollbarItem()
{
}

void ScrollbarItem::setMargins(int top, int bottom)
{
    // obviously, it mean right and left margin for horizontal bar
    if (top != -1)
        m_topMargin = top;
    if (bottom != -1)
        m_bottomMargin = bottom;
}

void ScrollbarItem::updateVisibilityAndFading(bool shouldFadeOut)
{
    show();

    if (opacity() != s_scrollbarOpacity)
        startFading(true);

    m_fadeOutTimeout.stop();
    if (shouldFadeOut)
        m_fadeOutTimeout.start();
}

void ScrollbarItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/, QWidget* /*widget*/)
{
    painter->setPen(QPen(QBrush(QColor(60, 60, 60)), 1));
    painter->setBrush(QColor(100, 100, 100));
    painter->drawRect(rect());
}

void ScrollbarItem::fadingFinished()
{
    // Hide when fading out.
    if (opacity() != s_scrollbarOpacity)
        hide();
}

void ScrollbarItem::startFadeOut()
{
    startFading(false);
}

void ScrollbarItem::startFading(bool in)
{
    m_fadeAnim.setDirection(in ? QAbstractAnimation::Forward : QAbstractAnimation::Backward);
    if (m_fadeAnim.state() != QAbstractAnimation::Running)
        m_fadeAnim.start();
}

/*!
  \a contentPos includes overshoot
*/

void ScrollbarItem::contentPositionUpdated(qreal contentPos, qreal contentLength, const QSizeF& viewSize, bool shouldFadeOut)
{
    qreal viewLength = m_orientation == Qt::Horizontal ? viewSize.width() : viewSize.height();

    if (contentLength < viewLength)
        contentLength = viewLength;

    qreal thumbRange = viewLength - m_topMargin - m_bottomMargin;

    qreal thumbPos = (thumbRange) * (-contentPos  / (contentLength));
    qreal thumbPosMax = (thumbRange) * (-contentPos + viewLength)  / (contentLength);

    thumbPos = qBound(static_cast<qreal>(0.), thumbPos, thumbRange - s_thumbMinSize);
    thumbPosMax = qBound(s_thumbMinSize, thumbPosMax, thumbRange);
    qreal thumbLength = thumbPosMax - thumbPos;

    if (m_orientation == Qt::Horizontal)
        setRect(QRectF(m_topMargin + thumbPos, viewSize.height() - s_thumbWidth - 1, thumbLength, s_thumbWidth));
    else
        setRect(QRectF(viewSize.width() - s_thumbWidth - 1, m_topMargin + thumbPos, s_thumbWidth,  thumbLength));

    // Show scrollbar only when scrolling is possible.
    if (thumbLength < thumbRange)
        updateVisibilityAndFading(shouldFadeOut);
    else if (isVisible())
        hide();
}
