#include "TileSelectionViewBase.h"
#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsWidget>
#include <QGraphicsRectItem>
#include <QTimer>
#include <QGraphicsSceneMouseEvent>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>

#include "ApplicationWindow.h"

#if USE_DUI
#include <DuiScene>
#endif

// FIXME: HomeView should be either a top level view, or just a central widget of the browsingview, probably the first one
// also, LAF is not finalized yet.

TileSelectionViewBase::TileSelectionViewBase(QGraphicsItem* parent, Qt::WindowFlags wFlags)
    : QGraphicsWidget(parent, wFlags)
    , m_slideAnimationGroup(new QParallelAnimationGroup())
    , m_bckg(new QGraphicsRectItem(rect(), this))
    , m_active(false)
{
    setFlag(QGraphicsItem::ItemClipsChildrenToShape, true);
    setFlag(QGraphicsItem::ItemClipsToShape, true);

    m_bckg->setPen(QPen(QBrush(QColor(10, 10, 10)), 3));
    m_bckg->setBrush(QColor(20, 20, 20));

    m_bckg->setZValue(0);
    connect(m_slideAnimationGroup, SIGNAL(finished()), this, SLOT(animFinished()));
}

TileSelectionViewBase::~TileSelectionViewBase()
{
    m_slideAnimationGroup->stop();
    m_slideAnimationGroup->clear();
    // FIXME leaking animation group. find out why it segfaults
//    delete m_slideAnimationGroup;
    delete m_bckg;
}

void TileSelectionViewBase::setGeometry(const QRectF& rect)
{
    QGraphicsWidget::setGeometry(rect);
    m_bckg->setRect(rect);
}

void TileSelectionViewBase::appear(ApplicationWindow *window)
{
    // FIXME: how to test if view is already in correct view?
    if (!scene()) {
        window->scene()->addItem(this);
        setZValue(100);
    }
    scene()->setActiveWindow(this);

    m_active = true;
    createViewItems();
    startInOutAnimation(m_active);
}

void TileSelectionViewBase::disappear()
{
    m_active = false;
    startInOutAnimation(m_active);
}

void TileSelectionViewBase::animFinished()
{
    // destroy thumbs when animation finished (outbound)
    if (m_active) {
        // set transparency
        m_bckg->setOpacity(0.8);
        emit appeared();
    } else {
        destroyViewItems();
        emit disappeared();
    }
}

void TileSelectionViewBase::tileItemActivated(TileItem* /*item*/)
{
    disappear();
}

void TileSelectionViewBase::startInOutAnimation(bool in)
{
    // ongoing?
    m_slideAnimationGroup->stop();
    m_slideAnimationGroup->clear();

    QPropertyAnimation* tabAnim = new QPropertyAnimation(this, "opacity");
    tabAnim->setDuration(500);
    tabAnim->setStartValue(in ? 0 : 1);
    tabAnim->setEndValue(in ? 1 : 0);
    tabAnim->setEasingCurve(QEasingCurve::OutCubic);
    m_slideAnimationGroup->addAnimation(tabAnim);
    m_slideAnimationGroup->start(QAbstractAnimation::KeepWhenStopped);
}

void TileSelectionViewBase::connectItem(TileItem& item)
{
    connect(&item, SIGNAL(itemActivated(TileItem*)), this, SLOT(tileItemActivated(TileItem*)));
    connect(&item, SIGNAL(itemClosed(TileItem*)), this, SLOT(tileItemClosed(TileItem*)));
    connect(&item, SIGNAL(itemEditingMode(TileItem*)), this, SLOT(tileItemEditingMode(TileItem*)));
}

