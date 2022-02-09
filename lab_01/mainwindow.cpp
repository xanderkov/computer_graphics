#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::get_var(double &var, const QLineEdit *lineEdit, const QString &err_msg)
{
    bool ok;
    var = lineEdit->text().toDouble(&ok);
    if (!ok)
        ui->statusbar->showMessage(err_msg, STATUS_BAR_TIMEOUT);
    return ok;
}

void MainWindow::on_addButton_clicked()
{
    double x, y;
    if (!(get_var(x, ui->xCoordEdit, "Invalid x") && get_var(y, ui->yCoordEdit, "Invalid y")))
        return;
    Point point(x, y);
    j_max = 0;
    /*
    if (points.contains(point))
    {
        ui->statusbar->showMessage("Point already exists", STATUS_BAR_TIMEOUT);
        return;
    }
    */
    points.push_back(point);
    int i = ui->tableWidget->rowCount();
    ui->tableWidget->insertRow(i);
    ui->tableWidget->setItem(i, 0, new QTableWidgetItem(QString::number(x)));
    ui->tableWidget->setItem(i, 1, new QTableWidgetItem(QString::number(y)));

    ui->statusbar->showMessage("Point added successful", STATUS_BAR_TIMEOUT);
}


void MainWindow::on_delButton_clicked()
{
    bool ok;
    int i = ui->indexEdit->text().toInt(&ok);
    if (!ok)
    {
        ui->statusbar->showMessage("Invalid index", STATUS_BAR_TIMEOUT);
        return;
    }
    if (i <= 0 || i > points.size())
    {
        ui->statusbar->showMessage("Index out of range", STATUS_BAR_TIMEOUT);
        return;
    }

    j_max = 0;
    points.remove(i - 1);

    ui->tableWidget->removeRow(i - 1);

    ui->statusbar->showMessage("Point deleted successful", STATUS_BAR_TIMEOUT);
}


void MainWindow::on_tableWidget_cellChanged(int row, int column)
{
}


void MainWindow::on_tableWidget_itemChanged(QTableWidgetItem *item)
{
    bool ok;
    double coord = item->text().toDouble(&ok);
    if (!ok)
    {
        if (item->column())
            ui->statusbar->showMessage("Invalid y", STATUS_BAR_TIMEOUT);
        else
            ui->statusbar->showMessage("Invalid x", STATUS_BAR_TIMEOUT);
        item->setText(QString::number(points[item->row()][item->column()]));
        return;
    }

    Point point(coord, coord);
    point[!item->column()] = points[item->row()][!item->column()];

    j_max = 0;
    points[item->row()][item->column()] = coord;

    ui->statusbar->showMessage("Point edited successful", STATUS_BAR_TIMEOUT);
}


void MainWindow::on_calcButton_clicked()
{
    if (j_max)
    {
        ui->statusbar->showMessage("Already calculated", STATUS_BAR_TIMEOUT);
        return;
    }
    if (points.size() < 7) {
        ui->statusbar->showMessage("Too few points", STATUS_BAR_TIMEOUT);
        return;
    }
    int i, j, k;
    angle_min = 1000;
    Line y_axis(1, 0, 0);
    Point rectCenter = Rectangle(points[0], points[1], points[2], points[3]).getRectCenter();
    for (i = 4; i < points.size() - 2; ++i)
        for (j = i + 1; j < points.size() - 1; ++j)
            for (k = j + 1; k < points.size(); ++k)
                if (!on_one_line(points[i], points[j], points[k]))
                {
                    Point incenter = Triangle(points[i], points[j], points[k]).incenter();
                    double cur_angle = getAngle(Line(rectCenter, incenter));
                    if (cur_angle < angle_min)
                    {
                        i_max = i, j_max = j, k_max = k;
                        incenterMax = incenter;
                        angle_min = cur_angle;
                    }

                }

    if (!j_max)
    {
        ui->statusbar->showMessage("No one triangle can be drawn", STATUS_BAR_TIMEOUT);
        return;
    }

    ui->statusbar->showMessage(
        "Triangle found successful on points "
        + QString::number(i_max + 1) + ", "
        + QString::number(j_max + 1) + " and "
        + QString::number(k_max + 1) + ". Angle: "
        + QString::number(angle_min * 180 / 3.1415926)
    );
    update();
}


double MainWindow::x_coord(double x) const
{
    return PAINT_WIDTH / 2 + (x - x_min - 0.5 * (x_max - x_min)) * scale_factor;
}

double MainWindow::y_coord(double y) const
{
    return PAINT_HEIGHT / 2 - (y - y_min - 0.5 * (y_max - y_min)) * scale_factor;
}

QPointF MainWindow::coord(const Point &point) const
{
    return QPointF(x_coord(point.x), y_coord(point.y));
}

QPen MainWindow::choosePen(int i) const
{
    if (i % 50 == 0)
        return QPen(Qt::black);
    if (i % 10 == 0)
        return QPen(Qt::darkGray);
    if (i % 5 == 0)
        return QPen(Qt::gray);
    return QPen(Qt::lightGray);
}

void MainWindow::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    int w = ui->centralwidget->width() - ui->tableWidget->width() - 30, h =  ui->centralwidget->height() - 20;
    painter.translate(ui->tableWidget->width() + 30, 10);
    painter.fillRect(0, 0, w, h, Qt::white);

    if (!j_max)
        return;

    // find scale factor
    x_max = qMax(qMax(points[i_max].x, points[j_max].x), qMax(points[k_max].x, incenterMax.x));
    y_max = qMax(qMax(points[i_max].y, points[j_max].y), qMax(points[k_max].y, incenterMax.y));
    x_min = qMin(qMin(points[i_max].x, points[j_max].x), qMin(points[k_max].x, incenterMax.x));
    y_min = qMin(qMin(points[i_max].y, points[j_max].y), qMin(points[k_max].y, incenterMax.y));
    double scale_x = 0.9 * PAINT_WIDTH / (x_max - x_min);
    double scale_y = 0.9 * PAINT_HEIGHT / (y_max - y_min);
    scale_factor = qMin(scale_x, scale_y);

    // draw points
    painter.setPen(Qt::red);
    painter.setBrush(Qt::red);
    foreach (const Point &point, points)
    {
        double x = x_coord(point.x);
        double y = y_coord(point.y);
        if (0 <= x && x <= PAINT_WIDTH && 0 <= y && y <= PAINT_HEIGHT)
        {
            painter.drawEllipse(x - 2, y - 2, 4, 4);
        }
    }

    // draw triangle
    painter.setPen(QPen(Qt::green, 3));
    painter.setBrush(QBrush(Qt::Dense4Pattern));
    painter.drawPolygon(QPolygonF() << coord(points[i_max]) << coord(points[j_max]) << coord(points[k_max]));

    Triangle triangle(points[i_max], points[j_max], points[k_max]);

    const QVector<int> indices = {
        i_max,
        j_max,
        k_max
    };

    const QVector<Line> sides =
    {
        Line(points[j_max], points[k_max]),
        Line(points[k_max], points[i_max]),
        Line(points[i_max], points[j_max])
    };

    const QVector<Line> altitudes =
    {
        Line(points[j_max], incenterMax),
        Line(points[k_max], incenterMax),
        Line(points[i_max], incenterMax)
    };

    const QVector<Point> Bs =
    {
        intersection(altitudes[0], sides[0]),
        intersection(altitudes[1], sides[1]),
        intersection(altitudes[2], sides[2])
    };

    painter.setPen(QPen(Qt::blue, 2));
    for (int i = 0; i != 3; ++i)
        painter.drawLine(coord(points[indices[i]]), coord(Bs[i]));
}

void MainWindow::drawLine(const Line &line, QPainter &painter)
{
    const double dx = x_max - x_min;
    const double dy = y_max - y_min;
    const double sx = PAINT_WIDTH / (dx * scale_factor);
    const double sy = PAINT_HEIGHT / (dy * scale_factor);
    const double cx = 0.5 * dx;
    const double cy = 0.5 * dy;
    const double rx = x_min + cx * (1 + sx);
    const double ry = y_min + cy * (1 + sy);
    const double lx = x_min + cx * (1 - sx);
    const double ly = y_min + cy * (1 - sy);

    const QVector<Line> borders =
    {
        Line(Point(lx, ly), Point(lx, ry)),
        Line(Point(lx, ly), Point(rx, ly)),
        Line(Point(rx, ry), Point(rx, ly)),
        Line(Point(rx, ry), Point(lx, ry))
    };

    QVector<Point> borders_borders;
    foreach (const Line &border, borders)
    {

        if (parallel(line, border))
            continue;

        Point point = intersection(line, border);
        if ((ly - EPS <= point.y && point.y <= ry + EPS) && (lx - EPS <= point.x && point.x <= rx + EPS) && !borders_borders.contains(point))
            borders_borders.push_back(point);
    }
    Q_ASSERT(borders_borders.size() == 2);
    painter.drawLine(coord(borders_borders[0]), coord(borders_borders[1]));
    borders_borders.clear();
}

