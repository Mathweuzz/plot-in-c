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

void tp_view_zoom_at(TP_View *v, double factor, double anchor_x, double anchor_y) {
    /* Evita ranges degenerados */
    if (factor <= 0.0) return;

    const double rx = (v->xmax - v->xmin);
    const double ry = (v->ymax - v->ymin);
    if (rx <= 0.0 || ry <= 0.0) return;

    /* Posição relativa do anchor dentro do viewport (0..1) */
    const double ax = (anchor_x - v->xmin) / rx;
    const double ay = (anchor_y - v->ymin) / ry;

    const double new_rx = rx * factor;
    const double new_ry = ry * factor;

    /* Mantém o anchor na mesma posição relativa */
    v->xmin = anchor_x - ax * new_rx;
    v->xmax = v->xmin + new_rx;

    v->ymin = anchor_y - ay * new_ry;
    v->ymax = v->ymin + new_ry;
}
