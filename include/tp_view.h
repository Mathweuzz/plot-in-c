#ifndef TP_VIEW_H
#define TP_VIEW_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TP_View {
    double xmin, xmax;
    double ymin, ymax;
} TP_View;

void tp_view_pan(TP_View *v, double dx, double dy);

/* factor < 1 => zoom in, factor > 1 => zoom out */
void tp_view_zoom(TP_View *v, double factor);

#ifdef __cplusplus
}
#endif

#endif
