#include "Widget_wrapper.h"

using namespace Layouting;

Widget_wrapper::Widget_wrapper(Widget *widget)
    : Widget(widget->asQObject())
    , m_widget(widget)
{
}

Widget_wrapper::~Widget_wrapper()
{
}

QWidget *Widget_wrapper::asQWidget() const
{
    return m_widget->asQWidget();
}

QSize Widget_wrapper::sizeHint() const {
    return m_widget->sizeHint();
}

QSize Widget_wrapper::minSize() const {
    return m_widget->minSize();
}

QSize Widget_wrapper::maxSizeHint() const {
    return m_widget->maxSizeHint();
}

QRect Widget_wrapper::geometry() const {
    return m_widget->geometry();
}

void Widget_wrapper::setGeometry(QRect geo) {
    m_widget->setGeometry(geo);
}

void Widget_wrapper::setParent(Widget *p) {
     m_widget->setParent(p);
}

QDebug& Widget_wrapper::dumpDebug(QDebug &d) const {
    return m_widget->dumpDebug(d);
}

bool Widget_wrapper::isVisible() const {
    return m_widget->isVisible();
}

void Widget_wrapper::setVisible(bool v) const {
    m_widget->setVisible(v);
}

std::unique_ptr<Widget> Widget_wrapper::parentWidget() const {
    return m_widget->parentWidget();
}

std::unique_ptr<Widget> Widget_wrapper::topLevel() const {
    return m_widget->topLevel();
}

void Widget_wrapper::setLayoutItem(Item *item) {
    m_widget->setLayoutItem(item);
}

void Widget_wrapper::show() {
    m_widget->show();
}

void Widget_wrapper::hide() {
    m_widget->hide();
}

void Widget_wrapper::close()
{
    m_widget->close();
}

void Widget_wrapper::move(int x, int y) {
    m_widget->move(x, y);
}

void Widget_wrapper::setSize(int width, int height) {
    m_widget->setSize(width, height);
}

void Widget_wrapper::setWidth(int width) {
    m_widget->setWidth(width);
}

void Widget_wrapper::setHeight(int height) {
    m_widget->setHeight(height);
}

void Widget_wrapper::update() {
    m_widget->update();
}

QPoint Widget_wrapper::mapFromGlobal(QPoint p) const {
    return m_widget->mapFromGlobal(p);
}

QPoint Widget_wrapper::mapToGlobal(QPoint p) const {
    return m_widget->mapToGlobal(p);
}

void Widget_wrapper::resize(QSize size)
{
   m_widget->resize(size);
}

void Widget_wrapper::onCloseEvent(QCloseEvent *ev)
{
    m_widget->onCloseEvent(ev);
}

bool Widget_wrapper::eventFilter(QEvent *ev)
{
    return m_widget->eventFilter(ev);
}

QSize Widget_wrapper::widgetMinSize(const QObject *o) const
{
    return m_widget->widgetMinSize(o);
}

QSize Widget_wrapper::widgetMaxSize(const QObject *o) const
{
    return m_widget->widgetMaxSize(o);
}

QString Widget_wrapper::id() const
{
    return m_widget->id();
}
