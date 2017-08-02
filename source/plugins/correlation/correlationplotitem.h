#ifndef CORRELATIONPLOTITEM_H
#define CORRELATIONPLOTITEM_H

#include "thirdparty/qcustomplot/qcustomplot_disable_warnings.h"
#include "thirdparty/qcustomplot/qcustomplot.h"
#include "thirdparty/qcustomplot/qcustomplot_enable_warnings.h"

#include <QQuickPaintedItem>
#include <QVector>
#include <QStringList>
#include <QVector>

class CorrelationPlotItem : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QVector<double> data MEMBER _data)
    Q_PROPERTY(double scrollAmount MEMBER _scrollAmount WRITE setScrollAmount NOTIFY scrollAmountChanged)
    Q_PROPERTY(double rangeSize READ rangeSize NOTIFY scrollAmountChanged)
    Q_PROPERTY(QVector<int> selectedRows MEMBER _selectedRows WRITE setSelectedRows)
    Q_PROPERTY(QVector<QColor> rowColors MEMBER _rowColors WRITE setRowColors)
    Q_PROPERTY(QStringList columnNames MEMBER _labelNames WRITE setLabelNames)
    Q_PROPERTY(QStringList rowNames MEMBER _graphNames)
    Q_PROPERTY(size_t columnCount MEMBER _columnCount WRITE setColumnCount)
    Q_PROPERTY(size_t rowCount MEMBER _rowCount)
    Q_PROPERTY(int elideLabelWidth MEMBER _elideLabelWidth WRITE setElideLabelWidth)
    Q_PROPERTY(bool showColumnNames MEMBER _showColumnNames WRITE setShowColumnNames)
    Q_PROPERTY(QRect viewport MEMBER _viewport)

public:
    explicit CorrelationPlotItem(QQuickItem* parent = nullptr);
    void paint(QPainter* painter);

    Q_INVOKABLE void savePlotImage(const QUrl& url, const QStringList& extensions);

protected:
    void routeMouseEvent(QMouseEvent* event);
    void routeWheelEvent(QWheelEvent* event);

    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void hoverMoveEvent(QHoverEvent* event);
    virtual void hoverLeaveEvent(QHoverEvent* event);
    virtual void mouseDoubleClickEvent(QMouseEvent* event);
    virtual void wheelEvent(QWheelEvent* event);

    void buildPlot();

private:
    const int MAX_SELECTED_ROWS_BEFORE_MEAN = 1000;

    QCPLayer* _textLayer = nullptr;
    QCPAbstractPlottable* _hoverPlottable = nullptr;
    QPointF _hoverPoint;
    QCPItemText* _hoverLabel = nullptr;
    QCPItemRect* _hoverColorRect = nullptr;
    QCPItemTracer* _itemTracer = nullptr;
    QRect _viewport;

    QFont _defaultFont9Pt;

    QCustomPlot _customPlot;
    size_t _columnCount = 0;
    size_t _rowCount = 0;
    int _elideLabelWidth = 120;
    QStringList _labelNames;
    QStringList _graphNames;
    QVector<double> _data;
    QVector<int> _selectedRows;
    QVector<QColor> _rowColors;
    bool _showColumnNames = true;
    double _scrollAmount = 0.0f;

    void populateMeanAveragePlot();
    void populateRawPlot();

    void refresh();

    void setSelectedRows(const QVector<int>& selectedRows);
    void setRowColors(const QVector<QColor>& rowColors);
    void setLabelNames(const QStringList& labelNames);
    void setElideLabelWidth(int elideLabelWidth);
    void setColumnCount(size_t columnCount);
    void setShowColumnNames(bool showColumnNames);
    void setScrollAmount(double scrollAmount);

    void scaleXAxis();
    double rangeSize();
    double columnLabelSize();
    double columnAxisWidth();

private slots:
    void onCustomReplot();
    void updateCustomPlotSize();
    void showTooltip();
    void hideTooltip();

signals:
    void rightClick();
    void scrollAmountChanged();
};
#endif // CORRELATIONPLOTITEM_H
