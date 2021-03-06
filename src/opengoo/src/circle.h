#ifndef CIRCLE_H
#define CIRCLE_H

#include <QVector2D>

class Circle
{
public:
    Circle(float x, float y, float radius)
        : center_(x, y), radius_(radius) {}
    Circle(const QVector2D &center, float radius)
        : center_(center), radius_(radius) {}
    Circle(const QPointF &center, float radius)
        : center_(center), radius_(radius) {}

    QVector2D center() const { return center_; }
    float radius() const { return radius_; }

    Circle operator / (float divisor)
    {
        return  Circle(center_ / divisor, radius_ / divisor);
    }

private:
    QVector2D center_;
    float radius_;
};

#endif // CIRCLE_H
