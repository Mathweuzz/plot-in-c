#include "tp_view.h"

void tp_view_pan(TP_View *v, double dx, double dy) {
    v->xmin += dx; v->xmax += dx;
    v->ymin += dy; v->ymax += dy;
}

void tp_view_zoom(TP_View *v, double factor) {
    const double cx = (v->xmin + v->xmax) * 0.5;
    const double cy = (v->ymin + v->ymax) * 0.5;

    const double half_w = (v->xmax - v->xmin) * 0.5 * factor;
    const double half_h = (v->ymax - v->ymin) * 0.5 * factor;

    v->xmin = cx - half_w;
    v->xmax = cx + half_w;
    v->ymin = cy - half_h;
    v->ymax = cy + half_h;
}
