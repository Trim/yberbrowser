#include <QtGui>

#include "CommonGestureRecognizer.h"
#include "EventHelpers.h"

#define DEBUG_EVENTS 1

static const int s_waitForClickTimeoutMS = 200;

//QApplication::startDragDistance() is too little (12)
// and causes clicks with finger to be interpreted as pans
static const int s_startPanDistance = 50;

/* TODO

    - This should go away

    - At max, this should synthetize TouchEvents

    - The event coords are not welldefined, since the events might
      sometimes come from the container (WebViewportItem), sometimes
      from the childs (Web View)

    - This should do mouse click filtering, as it is very unreliable
      on device.
 */

CommonGestureRecognizer::CommonGestureRecognizer(CommonGestureConsumer* consumer)
    : m_consumer(consumer)
    , m_delayedPressEvent(0)
    , m_delayedReleaseEvent(0)
    , m_pressDelay(300)

{
    reset();
}

CommonGestureRecognizer::~CommonGestureRecognizer()
{
    clearDelayedPress();
}

bool CommonGestureRecognizer::filterMouseEvent(QGraphicsSceneMouseEvent *event)
{
    // these events will be sent by this class, don't filter these
    if (m_delayedPressEvent == event || m_delayedReleaseEvent == event)
        return false;

    bool accepted = false;

    switch(event->type()) {
    case QEvent::GraphicsSceneMouseDoubleClick:
        clearDelayedPress();
        accepted = mouseDoubleClickEvent(event);
        break;
    case QEvent::GraphicsSceneMouseMove:
        accepted = mouseMoveEvent(event);
        break;
    case QEvent::GraphicsSceneMousePress:
        accepted = mousePressEvent(event);
        break;

    case QEvent::GraphicsSceneMouseRelease:
        accepted = mouseReleaseEvent(event);
        if (m_delayedPressEvent && !m_delayedReleaseEvent && accepted)
            captureDelayedPress(event, true);
        break;
    default:
        break;
    }

    return accepted;
}

void CommonGestureRecognizer::captureDelayedPress(QGraphicsSceneMouseEvent *event, bool wasRelease)
{
    Q_ASSERT(!m_delayedPressEvent && !wasRelease);
    Q_ASSERT(m_delayedPressEvent && !m_delayedReleaseEvent && wasRelease);

    if (m_pressDelay <= 0)
        return;

    QGraphicsSceneMouseEvent* delayedEvent = new QGraphicsSceneMouseEvent(event->type());
    copyMouseEvent(event, delayedEvent);
    delayedEvent->setAccepted(false);

    if (wasRelease) {
        m_delayedReleaseEvent = delayedEvent;

        // mouse press is more reliable than mouse release
        // we must send release with same coords as press
        copyMouseEventPositions(m_delayedPressEvent, m_delayedReleaseEvent);

        m_delayedPressTimer.start(m_pressDelay, this);
    } else {
        m_delayedPressEvent = delayedEvent;
        m_delayedPressMoment.start();
    }
}

void CommonGestureRecognizer::clearDelayedPress()
{
    if (m_delayedPressEvent) {
        m_delayedPressTimer.stop();
        delete m_delayedPressEvent;
        m_delayedPressEvent = 0;
        delete m_delayedReleaseEvent;
        m_delayedReleaseEvent = 0;
    }
}

void CommonGestureRecognizer::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_delayedPressTimer.timerId()) {
        m_delayedPressTimer.stop();
        m_consumer->tapGesture(m_delayedPressEvent, m_delayedReleaseEvent);
        clearDelayedPress();

    }
}

bool CommonGestureRecognizer::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
#if DEBUG_EVENTS
    qDebug() << __PRETTY_FUNCTION__ << event << event->screenPos();
#endif

    m_consumer->doubleTapGesture(event);
    return true;
}


bool CommonGestureRecognizer::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
#if DEBUG_EVENTS
    qDebug() << __PRETTY_FUNCTION__ << event << event->screenPos();
#endif

    if (event->button() != Qt::LeftButton)
        return false;

    if (!m_delayedPressEvent)
        captureDelayedPress(event);
    return true;
}

bool CommonGestureRecognizer::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
#if DEBUG_EVENTS
    qDebug() << __PRETTY_FUNCTION__ << event << event->screenPos();
#endif

    if (event->button() != Qt::LeftButton)
        return false;

    if (m_consumer->isPanning()) {
        m_consumer->stopPanGesture();
    } else if (m_delayedReleaseEvent) {
        // sometimes double click is lost if small mouse move occurs
        // inbetween
        QGraphicsSceneMouseEvent dblClickEvent(QEvent::GraphicsSceneMouseDoubleClick);
        copyMouseEvent(m_delayedPressEvent, &dblClickEvent);
        mouseDoubleClickEvent(&dblClickEvent);
    }

    return true;
}

bool CommonGestureRecognizer::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
#if DEBUG_EVENTS
    qDebug() << __PRETTY_FUNCTION__ << event << event->screenPos();
#endif

    bool isPanning = m_consumer->isPanning();

    if (!isPanning) {
        // if the button is still pressed
        if (m_delayedPressEvent && !m_delayedReleaseEvent) {
            QPoint d = m_delayedPressEvent->screenPos() - event->screenPos();
            if (qAbs(d.y()) > s_startPanDistance
                || qAbs(d.x()) > s_startPanDistance
                || m_delayedPressMoment.elapsed() > s_waitForClickTimeoutMS) {
                m_consumer->startPanGesture();
                isPanning = m_consumer->isPanning();

                // to avoid initial warping, don't use m_delayedPressEvent->screenPos()
                m_dragStartPos = event->screenPos();
                clearDelayedPress();
            }
        }
    }

    if (isPanning) {
        clearDelayedPress();
        m_consumer->panBy(event->screenPos() - m_dragStartPos);
        m_dragStartPos = event->screenPos();
        return true;
    }

    return false;
}

void CommonGestureRecognizer::reset()
{
    m_dragStartPos = QPoint();
    clearDelayedPress();
}

